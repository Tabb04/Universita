	.global Fucktorial
	.type Fucktorial, %function

	.text
	
	@n sta su r0

Fucktorial:	
	cmp r0, #0
	bgt continua
	mov r0, #1
	mov pc, lr

continua:
	push {r0, lr}
	sub r0, r0, #1
	bl Fucktorial
	pop {r1, lr}
	mul r0, r0, r1
	mov pc, lr
