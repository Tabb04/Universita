	.text
	.global cerca
	.type cerca, %function
	@cerco in un vettore, mi e' passato (x da cercare, *v vettore, n lunghezza vettore)
	@r0 = x, r1 = &v, r2 = n 	
	@resituisco -1 se non trovato altrimenti dico la posizione

cerca:	push {r4}
	mov r4, #0

for: 	ldr r3, [r1], #4	
	cmp r3, r0
	beq trovato
	add r4, r4, #1
	cmp r4, r2		@guardo se sono in fondo
	beq nontrovato	
 	b for

trovato:	mov r0, r4
		b cont

nontrovato: 	mov r0, #-1

cont:	pop {r4}
	mov pc, lr
