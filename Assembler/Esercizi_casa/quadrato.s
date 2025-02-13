	.text
	.global main

	@prendo 4 argomenti.

main:	push {lr, r4, r5, r6}

	ldr r0, [r1, #4] 	@a
	bl atoi
	mov r2, r0
	
	ldr r0, [r1, #8]	@b
	bl atoi
	mov r3, r0

	ldr r0, [r1, #12]	@c
	bl atoi
	mov r4, r0
	
	ldr r0, [r1, #16]	@x
	bl atoi
	mov r5, r0

	mul r6, r5, r5		@x^2
	mul r2, r2, r6		@a*x^2
	
	mul r3, r3, r5		@bx
	
	add r2, r2, r3		@a*x^2 + b*x
	add r2, r2, r4		@.. + c
	
	ldr r0, =v
	mov r1, r2
	pop {lr, r4, r5, r6}

	.data
v:	.string "Risultato e' %d"
	
	
	
