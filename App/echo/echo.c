#include "apilib.h"

void NeruMain(void)
{
	char cmdline[30], *p;
	api_cmdline(cmdline, 30);
	for (p = cmdline; *p > ' '; p++) { }	/* スペースが来るまで読み飛ばす */
	for (; *p == ' '; p++) { }	/* スペースを読み飛ばす */
	api_putstr0(p);
	api_end();
}
