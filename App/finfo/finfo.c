#include "apilib.h"

void NeruMain(void)
{
	int fh;
	char c, cmdline[30], *p;

	api_cmdline(cmdline, 30);
	for (p = cmdline; *p > ' '; p++) { }	/* スペースが来るまで読み飛ばす */
	for (; *p == ' '; p++) { }	/* スペースを読み飛ばす */

	fh = api_fopen("abc.txt");
	char s,t[20];
	int size = api_fsize(fh,0);
	sprintf(t, "File Name: %s\n", p);
	sprintf(s, "File Size: %d\n", size);
	api_putstr0(t);
	api_putstr0(s);
	api_end();
}
