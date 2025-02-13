	.data
fmt:	.string	"Il risultato vale %d\n"

	.text
	.global main

main:	mov r1, r0
	ldr r0, =fmt
	bl printf



