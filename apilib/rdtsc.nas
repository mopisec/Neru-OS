[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "rdtsc.nas"]

		GLOBAL	_io_rdtsc

[SECTION .text]

_io_rdtsc:	; void io_rdtsc(int *high, int *low);
		PUSHAD
		MOV		EBP,[ESP+36]
		DB		0x0F, 0x31			; RDTSC
		MOV		[EBP],EDX			;
		MOV		[EBP+4],EAX			;
		POPAD
		RET
