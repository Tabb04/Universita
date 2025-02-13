	.text
	.global main

main:	push {lr}
	push {r4}
	push {r5}
	push {r6}

	mov r6, r0
	ldr r3, [r1, #4]	@carico in r3 primo valore
	ldr r4, [r1, #8]	@carico in r4 secondo valore

	mov r0, r3
	bl atoi
	mov r5, r0		@in r6 ho primo valore in interi
	
	mov r0, r4
	bl atoi
	mov r2, r0
	
	mov r1, r5		
	ldr r0, =v 		@in r0 = stringa, r1 primo valore, r2 secondo


	bl printf
		
	pop {r6}
	pop {r5}
	pop {r4}
	pop {pc}
	
	.data
v:	.string "Primo valore e'%d, il secondo %d.\n"

