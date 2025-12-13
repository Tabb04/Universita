	.text
	.global main
	
	@in r0 input e output
		
main:
	mov r0, #3

fattoriale:
	push {r4, lr}
	mov r4, r0	
	cmp r4, #1		@sono nel caso base
	ble caso_base

	@passo ricorsivo
	sub r0, r4, #1		@faccio (n-1)	
	bl fattoriale		@chiamo fact(n-1)
	
	mul r0, r4, r0 		@faccio fact(n-1) * n
	
	b fine

caso_base:
	mov r0, #1

fine:	
	pop {r4, lr}
	bx lr
