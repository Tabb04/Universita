	.text
	.global conta
	.type conta, %function

	@in r0 ho l'indirizzo della string
	
		
main:	@test case con r0
	@ldr r0, =s		@testo con stringa ab23Abcd
	push {lr}
	push {r4}
	push {r5}		@uso per calcolo lett
	mov r4, #0		@numero occorrenze
	
for:	ldrb r2, [r0]
	cmp r2, #0		@guardo se sonk alla fine della stringa
	beq fine
	ldrb r3, [r0, #1]	@carattere dopo
	cmp r3, #0		@il carattere dopo e' la fine
	beq fine		
	
	add r5, r2, #1		@calcolo il carattere che sarebbe il primo + 1
	cmp r5, r3		@guardo se sono uguali
	beq uguali
	b dopo
	
uguali:	add r4, r4, #1		@aumento contatore
dopo:	add r0, r0, #1
	b for



fine:	mov r0, r4		@metto il risultato in 0r
	pop {r5}
	pop {r4}
	pop {lr}	

	@.data
@s:	@.string "ab23Abcd"
