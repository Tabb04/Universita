	.text
	.global main
	@invece di %s per stampare uso %d usando atoi per il primo parametro


main:	push {lr}		
	ldr r2, [r1, #4]

	push {r0}
	mov r0, r2
	bl atoi
	mov r2, r0
	pop {r0}
		
	mov r1, r0		
	ldr r0, =str		
	bl printf
	pop {pc}		

	.data
str:	.string	"Il numero di parametri e' %d e il primo e' %d\n"

