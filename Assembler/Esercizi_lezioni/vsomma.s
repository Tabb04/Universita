		.text
		.global main

main:		ldr r0, =v
		ldr r1, [r0]
		ands r2, r1, #1
		beq then
		sub r1, r1, #1
		b oltrethen

then:		add r1, r1, #1

oltrethen: 	str r1, [r0]
		
		mov pc, lr

		.data
v:		.word 12, 1, 7, -3
	
	
	
	
