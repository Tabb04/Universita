	.text
	.global main
	@alloco spazio per n vettori passati in r0	
	
main:	@r0 = n
	mov r0, #4
	push {r0, lr}
	lsl r0, r0, #2
	
	bl malloc
	cmp r0, #0		@non ha funzionato malloc
	beq fine
	
	pop {r1}		@in r1 dimensione vettore
	push {r0}		@mi salvo l'indirizzo di r0

	mov r2, #1		@1 da memorizzare
	
while:	cmp r1, #0
	beq fine		@vuol dire ho finito iterazioni, n = 0
	str r2, [r0], #4	@scrivo in r0[0] 1 e poi post incremento
	sub r1, r1, #1
	b while

fine:	pop {r0}		@indirizzo del vettore allocato
	pop {pc}
	
