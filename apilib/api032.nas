[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api032.nas"]

        GLOBAL  _api_osselect

[SECTION .text]

_api_osselect:  ; void api_osselect(int i);
        MOV     EDX,30
        MOV     EAX,[ESP+4]   ; i
        INT     0x40
        RET
