#include <stdio.h>   /* sprintf */
#include "apilib.h"

void readrtc(unsigned char *t)
{
	char err;
	char *buf[12];
	unsigned char s[24], t[7];
	static unsigned char adr[7] = { 0x00, 0x02, 0x04, 0x07, 0x08, 0x09, 0x32 };
	static unsigned char max[7] = { 0x60, 0x59, 0x23, 0x31, 0x12, 0x99, 0x99 };
	int i;
	api_initmalloc();
	buf = api_malloc(150 * 50);
	win = api_openwin(buf, 150, 50, -1, "Clock");
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
	sprintf(s, "%02X%02X.%02X.%02X %02X:%02X:%02X\n", t[6], t[5], t[4], t[3], t[2], t[1], t[0]);
	api_boxfilwin(win, 28, 27, 115, 41, 7 /* 白 */);
	api_putstrwin(win, 28, 27, 0 /* 黒 */, 11, s);
	}
	api_end();
}
