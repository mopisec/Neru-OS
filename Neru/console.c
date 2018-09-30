/* コンソール関係 MEMO
cursorx - 8 で一文字戻る
cursory + 16 で次の行
横30まで入力可能
縦8まで入力可能
*/

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

unsigned char t[7];
char serialstr[0];

void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[30];
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;	/* 未使用マーク */
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {	/* 日本語フォントファイルを読み込めたか？ */
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	/* プロンプト表示 */
	cons_putchar(&cons, '$', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) { /* カーソル用タイマ */
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0); /* 次は0を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_00FF00;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1); /* 次は1を */
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {	/* カーソルON */
				cons.cur_c = COL8_00FF00;
			}
			if (i == 3) {	/* カーソルOFF */
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {	/* コンソールの「×」ボタンクリック */
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
				if (i == 8 + 256) {
					/* バックスペース */
					if (cons.cur_x > 16) {
						/* カーソルをスペースで消してから、カーソルを1つ戻す */
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					/* Enter */
					/* カーソルをスペースで消してから改行する */
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);	/* コマンド実行 */
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					/* プロンプト表示 */
					cons_putchar(&cons, '$', 1);
				} else {
					/* 一般文字 */
					if (cons.cur_x < 240) {
						/* 一文字表示してから、カーソルを1つ進める */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			/* カーソル再表示 */
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c,
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {	/* タブ */
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_00FF00, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (s[0] == 0x0a) {	/* 改行 */
		cons_newline(cons);
	} else if (s[0] == 0x0d) {	/* 復帰 */
		/* とりあえずなにもしない */
	} else {	/* 普通の文字 */
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_00FF00, COL8_000000, s, 1);
		}
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 240) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	if (cons->cur_y < 28 + 608) {
		cons->cur_y += 16; /* 次の行へ */
	} else {
		/* スクロール */
		if (sheet != 0) {
			for (y = 28; y < 28 + 608; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			for (y = 28 + 608; y < 28 + 608; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 608);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "clock") == 0 && cons->sht != 0) {
		cmd_clock(cons);
	} else if (strcmp(cmdline, "clear") == 0 && cons->sht != 0) {
		cmd_clear(cons);
	} else if (strcmp(cmdline, "writecmos") == 0 && cons->sht != 0) {
		cmd_writecmos(cons);
	} else if (strcmp(cmdline, "ls") == 0 && cons->sht != 0) {
		cmd_ls(cons);
  } else if (strncmp(cmdline, "rm", 15) == 0) {
    cmd_rm(cons, cmdline);
  } else if (strncmp(cmdline, "hikisu", 6) == 0) {
    cmd_hikisu(cons, cmdline);
  } else if (strcmp(cmdline, "reset") == 0 && cons->sht != 0) {
		cmd_reset(cons);
	} else if (strcmp(cmdline, "serial") == 0 && cons->sht != 0) {
		cmd_serial();
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			/* \83R\83}\83\93\83h\82ł͂Ȃ\AD\81A\83A\83v\83\8A\82ł\E0\82Ȃ\AD\81A\82\B3\82\E7\82ɋ\F3\8Ds\82ł\E0\82Ȃ\A2 */
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

void cmd_reset(struct CONSOLE *cons)
{
	io_cli();
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, 0xfe);
	for (;;) io_hlt();
}

void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];
	sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
	return;
}

void cmd_clock(struct CONSOLE *cons)
{
	unsigned char s[24], t[7];
	readrtc(t);
	sprintf(s, "%02X%02X.%02X.%02X %02X:%02X:%02X\n", t[6], t[5], t[4], t[3], t[2], t[1], t[0]);
	cons_putstr0(cons, s);
	return;
}

void cmd_serial(void)
{
	unsigned char s[7];
	sprintf(s, "%x",PCICheckVendor(0,0));
	write_serial_string(s);
	return;
}

void cmd_writecmos(struct CONSOLE *cons)
{
	io_out8(0x70, 0x00);
	io_out8(0x71, 0x00);
	io_out8(0x70, 0x02);
	io_out8(0x71, 0x14);
	io_out8(0x70, 0x04);
	io_out8(0x71, 0x14);
	return;
}

void cmd_clear(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 608; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 608);
	cons->cur_y = 28;
	return;
}

void cmd_ls(struct CONSOLE *cons)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_rm(struct CONSOLE *cons, char *cmdline)
{
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i;
    char name[18];
    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0;
    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    finfo->name[0] = 0xe5;
    return;
}

void cmd_hikisu(struct CONSOLE *cons, char *cmdline)
{
	/*
	int i, t;
	char o[18], name[20];
	for (i = 0; i < 13; i++) {
			if (cmdline[i] <= ' ') {
					break;
			}
			o[i] = cmdline[i];
	}
	for (i = 7, t = 0; i < 18; i++, t++) {
		name[t] = o[i];
	}
	*/
	cons_putstr0(cons, cmdline);
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();
	if (cons->sht != 0) {
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);	/* 768\81`1023 */
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024\81`2023 */
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	/* \83R\83}\83\93\83h\83\89\83C\83\93\82ɓ\FC\97͂\B3\82ꂽ\95\B6\8E\9A\97\F1\82\F0\81A\88ꕶ\8E\9A\82\B8\82V\82\B5\82\A2\83R\83\93\83\\81[\83\8B\82ɓ\FC\97\CD */
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	/* \83R\83}\83\93\83h\83\89\83C\83\93\82ɓ\FC\97͂\B3\82ꂽ\95\B6\8E\9A\97\F1\82\F0\81A\88ꕶ\8E\9A\82\B8\82V\82\B5\82\A2\83R\83\93\83\\81[\83\8B\82ɓ\FC\97\CD */
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET *sht;

	/* \83R\83}\83\93\83h\83\89\83C\83\93\82\A9\82\E7\83t\83@\83C\83\8B\96\BC\82𐶐\AC */
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0; /* \82Ƃ肠\82\A6\82\B8\83t\83@\83C\83\8B\96\BC\82̌\E3\82\EB\82\F00\82ɂ\B7\82\E9 */

	/* \83t\83@\83C\83\8B\82\F0\92T\82\B7 */
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		/* \8C\A9\82\A9\82\E7\82Ȃ\A9\82\C1\82\BD\82̂Ō\E3\82\EB\82\C9".HRB"\82\F0\82\AF\82Ă\E0\82\A4\88\EA\93x\92T\82\B5\82Ă݂\E9 */
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		/* \83t\83@\83C\83\8B\82\AA\8C\A9\82\A9\82\C1\82\BD\8Fꍇ */
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					/* \83A\83v\83\8A\82\AA\8AJ\82\AB\82\C1\82ςȂ\B5\82ɂ\B5\82\BD\89\BA\82\B6\82\AB\82𔭌\A9 */
					sheet_free(sht);	/* \95\B6\82\E9 */
				}
			}
			for (i = 0; i < 8; i++) {	/* \83N\83\8D\81[\83Y\82\B5\82ĂȂ\A2\83t\83@\83C\83\8B\82\F0\83N\83\8D\81[\83Y */
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	/* \83t\83@\83C\83\8B\82\AA\8C\A9\82\A9\82\E7\82Ȃ\A9\82\C1\82\BD\8Fꍇ */
	return 0;
}

void sysclock_task(void)
{
	int i, j;
	char err, cnt;
	unsigned char s[6];
	static unsigned char adr[7] = { 0x00, 0x02, 0x04, 0x07, 0x08, 0x09, 0x32 };
	static unsigned char max[7] = { 0x60, 0x59, 0x23, 0x31, 0x12, 0x99, 0x99 };
	struct TASK *task = task_now();
	struct TIMER *clock_timer = timer_alloc();
	timer_init(clock_timer, &task->fifo, 1);
	timer_settime(clock_timer, 100);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i == 1) {
				for (cnt = 0; cnt < 3; cnt++) {
					err = 0;
					for (j = 0; j < 7; j++) {
						io_out8(0x70, adr[j]);
						t[j] = io_in8(0x71);
					}
					for (j = 0; j < 7; j++) {
						io_out8(0x70, adr[j]);
						if (t[j] != io_in8(0x71) || (t[j] & 0x0f) > 9 || t[j] > max[j]) {
							err = 1;
						}
					}
					if (err == 0) {
						break;
					}
				}
				struct SHTCTL *ctl = (struct SHTCTL *) *((int *) 0x0fe4);
				struct SHEET *sht = ctl->sheets[0];
				sprintf(s, "%02X:%02X\0", t[2], t[1]);
				putfonts8_asc_sht(sht, 977, 6, COL8_000000, COL8_C6C6C6, s, 5);
				putfonts8_asc_sht(sht, 17, 6, COL8_000000, COL8_C6C6C6, "Neru", 4);
				timer_settime(clock_timer, 100);
			}
		}
	}
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	char name[18], *p, *q;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1;	/* eax\82̎\9F\82̔Ԓn */
		/* \95ۑ\B6\82̂\BD\82߂\CCPUSHAD\82\F0\8B\AD\88\F8\82ɏ\91\82\AB\8A\B7\82\A6\82\E9 */
		/* reg[0] : EDI,   reg[1] : ESI,   reg[2] : EBP,   reg[3] : ESP */
		/* reg[4] : EBX,   reg[5] : EDX,   reg[6] : ECX,   reg[7] : EAX */
	int i;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	if (edx == 1) {
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {
		cons_putstr0(cons, (char *) ebx + ds_base);
	} else if (edx == 3) {
		cons_putstr1(cons, (char *) ebx + ds_base, ecx);
	} else if (edx == 4) {
		return &(task->tss.esp0);
	} else if (edx == 5) {
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
		sheet_updown(sht, shtctl->top); /* \8D\A1\82̃}\83E\83X\82Ɠ\AF\82\B6\8D\82\82\B3\82ɂȂ\E9\82悤\82Ɏw\92\E8\81F \83}\83E\83X\82͂\B1\82̏\E3\82ɂȂ\E9 */
		reg[7] = (int) sht;
	} else if (edx == 6) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {
		memman_init((struct MEMMAN *) (ebx + ds_base));
		ecx &= 0xfffffff0;	/* 16\83o\83C\83g\92P\88ʂ\C9 */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16\83o\83C\83g\92P\88ʂɐ؂\E8\8Fグ */
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {
		ecx = (ecx + 0x0f) & 0xfffffff0; /* 16\83o\83C\83g\92P\88ʂɐ؂\E8\8Fグ */
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if ((ebx & 1) == 0) {
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {
		sht = (struct SHEET *) ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
	} else if (edx == 13) {
		sht = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if ((ebx & 1) == 0) {
			if (eax > esi) {
				i = eax;
				eax = esi;
				esi = i;
			}
			if (ecx > edi) {
				i = ecx;
				ecx = edi;
				edi = i;
			}
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {
		sheet_free((struct SHEET *) ebx);
	} else if (edx == 15) {
		for (;;) {
			io_cli();
			if (fifo32_status(&task->fifo) == 0) {
				if (eax != 0) {
					task_sleep(task);	/* FIFO\82\AA\8B\F3\82Ȃ̂ŐQ\82đ҂\C2 */
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons->sht != 0) { /* \83J\81[\83\\83\8B\97p\83^\83C\83} */
				/* \83A\83v\83\8A\8E\C0\8Ds\92\86\82̓J\81[\83\\83\8B\82\AA\8Fo\82Ȃ\A2\82̂ŁA\82\A2\82\E0\8E\9F\82͕\\8E\A6\97p\82\CC1\82𒍕\B6\82\B5\82Ă\A8\82\AD */
				timer_init(cons->timer, &task->fifo, 1); /* \8E\9F\82\CD1\82\F0 */
				timer_settime(cons->timer, 50);
			}
			if (i == 2) {	/* \83J\81[\83\\83\8BON */
				cons->cur_c = COL8_00FF00;
			}
			if (i == 3) {	/* \83J\81[\83\\83\8BOFF */
				cons->cur_c = -1;
			}
			if (i == 4) {	/* \83R\83\93\83\\81[\83\8B\82\BE\82\AF\82\F0\95\B6\82\E9 */
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);	/* 2024\81`2279 */
				cons->sht = 0;
				io_sti();
			}
			if (i >= 256) { /* \83L\81[\83{\81[\83h\83f\81[\83^\81i\83^\83X\83NA\8Co\97R\81j\82Ȃ\C7 */
				reg[7] = i - 256;
				return 0;
			}
		}
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;	/* \8E\A9\93\AE\83L\83\83\83\93\83Z\83\8B\97L\8C\F8 */
	} else if (edx == 17) {
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {
		timer_settime((struct TIMER *) ebx, eax);
	} else if (edx == 19) {
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {
		if (eax == 0) {
			i = io_in8(0x61);
			io_out8(0x61, i & 0x0d);
		} else {
			i = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i & 0xff);
			io_out8(0x42, i >> 8);
			i = io_in8(0x61);
			io_out8(0x61, (i | 0x03) & 0x0f);
		}
	} else if (edx == 21) {
		for (i = 0; i < 8; i++) {
			if (task->fhandle[i].buf == 0) {
				break;
			}
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if (i < 8) {
			finfo = file_search((char *) ebx + ds_base,
					(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int) fh;
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
		}
	} else if (edx == 22) {
		fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
	} else if (edx == 23) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}
		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (edx == 24) {
		fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] = fh->pos - fh->size;
		}
	} else if (edx == 25) {
		fh = (struct FILEHANDLE *) eax;
		for (i = 0; i < ecx; i++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	} else if (edx == 26) {
		i = 0;
		for (;;) {
			*((char *) ebx + ds_base + i) =  task->cmdline[i];
			if (task->cmdline[i] == 0) {
				break;
			}
			if (i >= ecx) {
				break;
			}
			i++;
		}
		reg[7] = i;
	} else if (edx == 27) {
		reg[7] = task->langmode;
	} else if (edx == 30) {
		fh = (struct FILEHANDLE *) reg[7];
		hrb_api_osselect(fh->buf, (int *) memman_alloc_4k(memman, 512 * 1024));
	} else if (edx == 31) {		/* sendkey */
        for (p = (unsigned char *) ebx + ds_base; *p != 0; p++) {
            if (*p != 0x0d) {
                io_cli();
                fifo32_put(&task->fifo, *p + 256);
                io_sti();
            }
        }
    }
	return 0;
}

void hrb_api_osselect(char *bhs, int *tmp)
/* 主な作業内容：
    bootpackのコード部分をtmpのメモリにコピーする
    割り込み禁止にする
    0xfff8のセレクタを設定する
    bhsの内容を0x280000にメモしておく
    far-jmp 0xfff8:hrb_api_osselect_second
*/
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    int i, *bootpack = (int *) ADR_BOTPAK;
    for (i = 0; i < (LIMIT_BOTPAK + 1) / 4; i++) {
        tmp[i] = bootpack[i];
    }
    /* PICはまた初期化されるのでそのための準備 */
    io_out8(PIC0_IMR, 0xff); /* すべて禁止 */
    io_out8(PIC1_IMR, 0xff); /* すべて禁止 */
    io_cli();
    set_segmdesc(gdt + 8191, LIMIT_BOTPAK, (int) tmp, AR_CODE32_ER);
    *((int *) ADR_BOTPAK) = (int) bhs;
    farjmp((int) hrb_api_osselect_second, 8191 * 8);
}

void hrb_api_osselect_second(void)
/* 主な作業内容：
    bhsのコードを0x280000に転送
    bhsのデータもヘッダを解析して転送
    asm_osselect_third(esp);
        → ESP=esp; farjmp(0x1b, 2 * 8);
*/
{
    char *bhs = (char *) *((int *) ADR_BOTPAK);
    int i, *bootpack = (int *) ADR_BOTPAK, *bhs_i = (int *) bhs;
    int *p, *q, datsiz;
    for (i = 0; i < (LIMIT_BOTPAK + 1) / 4; i++) {
        bootpack[i] = bhs_i[i];
    }
    datsiz = (bhs_i[16 / 4] + 3) / 4;
    p = (int *) (bhs_i[20 / 4] + bhs);
    q = (int *) bhs_i[12 / 4];
    for (i = 0; i < datsiz; i++) {
        q[i] = p[i];
    }
    asm_osselect_third(bhs_i[12 / 4]);
}

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* \88ُ\ED\8FI\97\B9\82\B3\82\B9\82\E9 */
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);	/* \88ُ\ED\8FI\97\B9\82\B3\82\B9\82\E9 */
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

void readrtc(unsigned char *t)
{
    char err;
    static unsigned char adr[7] = { 0x00, 0x02, 0x04, 0x07, 0x08, 0x09, 0x32 };
    static unsigned char max[7] = { 0x60, 0x59, 0x23, 0x31, 0x12, 0x99, 0x99 };
    int i;
    for (;;) { /* 読み込みが成功するまで繰り返す */
        err = 0;
        for (i = 0; i < 7; i++) {
            io_out8(0x70, adr[i]);
            t[i] = io_in8(0x71);
        }
        for (i = 0; i < 7; i++) {
            io_out8(0x70, adr[i]);
            if (t[i] != io_in8(0x71) || (t[i] & 0x0f) > 9 || t[i] > max[i]) {
                err = 1;
            }
        }
        if (err == 0) {
            return;
        }
    }
}
