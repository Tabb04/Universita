	.text
	.global main

main:	push {lr}
	mov r3, r0
	ldr r0, [r1, #4]
	bl atoi
	mov r3, r0		@in r3 = primo valore
	ldr r0, [r1, #8]
	bl atoi	
	mov r1, r3		@in r1 = primo valore
	mov r2, r0		@in r2 = secondo valore
	
	ldr r0, =v 		@in r0 = stringa
	
	bl printf
	
	pop {pc}
	
	.data
v:	.string "Primo valore e'%d\n"
	
