#include "apilib.h"

void NeruMain(void)
{
    char s[32], *p;
    int i;

    /* �R�}���h���C����� */
    api_cmdline(s, 30);
    for (p = s; *p > ' '; p++) { } /* �X�y�[�X������܂œǂݔ�΂� */
    for (; *p == ' '; p++) { }     /* �X�y�[�X��ǂݔ�΂� */
    i = api_fopen(p);
    if (i == 0) {
        api_putstr0("file open error!\n");
        api_end();
    }
    api_osselect(i);
}
