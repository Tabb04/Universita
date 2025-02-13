	.text
	.global main

main:
	mov r0, #123
	mov r1, r0
	cmp r0, r1
	beq fine
	sub r1, r1, #1	
	b ritorno

fine:
	sub r1, r1, #2
ritorno:
	mov pc, lr
