	.global main

main:	push {lr}
	mov r2, r0
	ldr r0, [r1, #4]
	bl atoi
	mov r1, r0
	ldr r0, =v
	bl printf
	pop {pc}
	
	
	.data
v:	.string	"Numero input e' %d\n"
