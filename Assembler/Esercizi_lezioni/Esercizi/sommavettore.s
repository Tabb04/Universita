	.global main

main:
	ldr r0, =v
	mov r1, #4
	mov r2, #0

loop:	ldr r3, [r0],#4
	add r2, r2, r3
	sub r1, r1, #1
	cmp r1, #0
	bne loop

	.data
v:	.word 12, 1, 7, -3
	
	
	
	
