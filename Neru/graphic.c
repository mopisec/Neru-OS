/* �O���t�B�b�N�����֌W */

#include "bootpack.h"

unsigned int table_8_32[256];
char wp[] = "neruos.jpg";

void init_palette(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:�� */
		0xff, 0x00, 0x00,	/*  1:���邢�� */
		0x00, 0xff, 0x00,	/*  2:���邢�� */
		0xff, 0xff, 0x00,	/*  3:���邢���F */
		0x00, 0x00, 0xff,	/*  4:���邢�� */
		0xff, 0x00, 0xff,	/*  5:���邢�� */
		0x00, 0xff, 0xff,	/*  6:���邢���F */
		0xff, 0xff, 0xff,	/*  7:�� */
		0xc6, 0xc6, 0xc6,	/*  8:���邢�D�F */
		0x84, 0x00, 0x00,	/*  9:�Â��� */
		0x00, 0x84, 0x00,	/* 10:�Â��� */
		0x84, 0x84, 0x00,	/* 11:�Â����F */
		0x00, 0x00, 0x84,	/* 12:�Â��� */
		0x84, 0x00, 0x84,	/* 13:�Â��� */
		0x00, 0x84, 0x84,	/* 14:�Â����F */
		0x84, 0x84, 0x84	/* 15:�Â��D�F */
	};
	unsigned char table2[216 * 3];
	int i, r, g, b;
	if(binfo->vmode == 8) {
		set_palette(0, 15, table_rgb);
		for (b = 0; b < 6; b++) {
			for (g = 0; g < 6; g++) {
				for (r = 0; r < 6; r++) {
					table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
					table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
					table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
				}
			}
		}
		set_palette(16, 231, table2);
	} else { // 32bpp
		for (i = 0; i < 16; i++) {
			r = table_rgb[i * 3 + 0];
			g = table_rgb[i * 3 + 1];
			b = table_rgb[i * 3 + 2];
			table_8_32[i] = (r<<16)|(g<<8)|b;
		}
		for (b = 0; b < 6; b++) {
			for (g = 0; g < 6; g++) {
				for (r = 0; r < 6; r++) {
					table_8_32[r + g*6 + b*36 + 16] = ((r*51)<<16)|((g*51)<<8)|(b*51);
				}
			}
		}
	}
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();	/* ���荞�݋��t���O�̒l���L�^���� */
	io_cli(); 					/* ���t���O��0�ɂ��Ċ��荞�݋֎~�ɂ��� */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* ���荞�݋��t���O�����ɖ߂� */
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			//x0��x�Ƃ��āAx1��x�ȏ��ł����ԁA
			vram[y * xsize + x] = c;
	}
	return;
}

void init_screen8(char *vram, int x, int y)
{

	//�f�X�N�g�b�v�w�i
	boxfill8(vram, x, COL8_008484,  0,     0,      x, y);
	//���o�[
	boxfill8(vram, x, COL8_C6C6C6, 0, 0, 1023, 25);
	//���o�[���v
	boxfill8(vram, x, COL8_848484, 975, 3, 1018, 3);
	boxfill8(vram, x, COL8_848484, 975, 4, 925, 23);
	boxfill8(vram, x, COL8_FFFFFF, 975, 24, 1018, 24);
	boxfill8(vram, x, COL8_FFFFFF, 1019, 3, 1019, 24);
/*
	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
*/
	boxfill8(vram, x, COL8_FFFFFF,  3, 3, 59,     3);
	boxfill8(vram, x, COL8_FFFFFF,  2, 3,  2,     23);
	boxfill8(vram, x, COL8_848484,  3, 23, 59,     23);
	boxfill8(vram, x, COL8_848484, 59, 4, 59,     22);
	boxfill8(vram, x, COL8_000000,  2, 24, 59,     24);
	boxfill8(vram, x, COL8_000000, 60, 3, 60,     24);
	return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	struct TASK *task = task_now();
	char *nihongo = (char *) *((int *) 0x0fe8), *font;
	int k, t;

	if (task->langmode == 0) {
		for (; *s != 0x00; s++) {
			putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
			x += 8;
		}
	}
	if (task->langmode == 1) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
					k = (task->langbyte1 - 0x81) * 2;
				} else {
					k = (task->langbyte1 - 0xe0) * 2 + 62;
				}
				if (0x40 <= *s && *s <= 0x7e) {
					t = *s - 0x40;
				} else if (0x80 <= *s && *s <= 0x9e) {
					t = *s - 0x80 + 63;
				} else {
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font     );	/* ������ */
				putfont8(vram, xsize, x    , y, c, font + 16);	/* �E���� */
			}
			x += 8;
		}
	}
	if (task->langmode == 2) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (0x81 <= *s && *s <= 0xfe) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
				}
			} else {
				k = task->langbyte1 - 0xa1;
				t = *s - 0xa1;
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font     );	/* ������ */
				putfont8(vram, xsize, x    , y, c, font + 16);	/* �E���� */
			}
			x += 8;
		}
	}
	return;
}

void init_mouse_cursor8(char *mouse, char bc)
/* �}�E�X�J�[�\���������i16x16�j */
{
	static char cursor[16][16] = {
		"*...............",
		"**..............",
		"*O*.............",
		"*OO*............",
		"*OOO*...........",
		"*OOOO*..........",
		"*OOOOO*.........",
		"*OOOOOO*........",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OO*OOO*OO*.....",
		"*O**OOO**O*.....",
		".*..*OO*.*......",
		".....*O*........",
		"......*.........",
		"................"
	};
	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
