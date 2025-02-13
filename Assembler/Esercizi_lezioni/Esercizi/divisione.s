	.text
	.global main
	@divido un numero while(x>1)	

main:	mov r0, #40

	cmp r0, #1
	ble fine

loop:	lsr r0, #1
	cmp r0, #1
	bgt loop

fine:	mov pc, lr
