#include "apilib.h"

void NeruMain(void)
{
    char s[32], *p;
    int i;

    /* コマンドライン解析 */
    api_cmdline(s, 30);
    for (p = s; *p > ' '; p++) { } /* スペースが来るまで読み飛ばす */
    for (; *p == ' '; p++) { }     /* スペースを読み飛ばす */
    i = api_fopen(p);
    if (i == 0) {
        api_putstr0("file open error!\n");
        api_end();
    }
    api_osselect(i);
}
