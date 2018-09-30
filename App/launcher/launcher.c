#include "apilib.h"

void NeruMain(void)
{
	int win;
	char buf[216 * 237];
	win = api_openwin(buf, 216, 237, -1, "launcher");
	api_boxfilwin(win, 8, 29, 207, 228, 0);
	
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			api_sendkey("start bball\n");
		}
	}
}
