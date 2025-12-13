	.data
fmt:	.string "Il risultato vale %d\n"

	.text
	.global main

main:	mov r0, #5
	mov r1, #4
	add r3, r1, r0

	push {lr}
	ldr r0, =fmt
	mov r1, r3
	bl printf 
	mov r0, #0	
	pop {pc}

