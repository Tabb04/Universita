	.text
	.global conta_stringhe
	.type conta_stringhe, %function

	@in r0 ho l'indirizzo iniziale
	@presuppongo che in r5 ho il numero di stringhe anche se non e' vero
	
conta_stringhe:	push {lr}
		push {r4}	@il mio contatore
		push {r5}
		mov r5, @numero stringhe	
		mov r4, #0
	

for:		@caso finito	@farei con r1 = 0 e finirei
		@beq finito		
	
		ldr r3, [r0], #4	@indirizzo prima stringa
		mov r0, r3
		bl conta

		add r4, r4, r0		@aggiungo risultato di conta
		@sub r5, r5, #1		@sottraggo 1 al numero di stringhe
		b for

fine:		mov r0, r4		@metto ris in r0
		pop {r5}
		pop {r4}
		pop {pc}


