	.text
	.global main
	
	@va in loop
	
main:	@r0 = n
	mov r0, #3
loop:	
	cmp r0, #1		@controllo caso base
	movle pc, lr
	
	push {lr}
	sub r0, r0, #1
	push {r0}		@salvo n-1
	bl loop
	pop {r1}		@ mi riprendo n-1
	push {r0}		@ mi salvo fib (n-1)

	sub r0, r1, #1
	bl loop
	pop {r1}
	add r0, r0, r1
	mov r1, r0
	ldr r0, =v
	bl printf
	pop {pc}		
	

	.data
v:	.string "Il risultato (sbagliato) e' %d"
	
		
