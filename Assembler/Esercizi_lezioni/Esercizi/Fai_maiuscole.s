	.global main
	@utilizzo le tabelle ascii
	@se voglio visualizzare guardo contenuto della stringa guardo indirizzo di r3 con x/numc

main:	ldr r0, =s
	mov r3, r0		@indirizzo della prima lettera
	mov r2, #0
	sub r2, r2, #'a'
	add r2, r2, #'A'

loop:	ldrb r1, [r0] 		@oppure ldrb r1, [r0], #1 ; avanzo per byte
	cmp r0, #0		@codifica carattere NULL
	beq fine
	cmp r1, #97		@97 e' la codifica di a minuscola, se il carattere viene prima nn interess
	blt cont
	cmp r1, #'z'		@fa stessa cosa dell'a
	bgt cont
	add r1, r1, r2
	strb r1, [r0]		@oppure ldrb r1, [r0-#1] ma è più lungo inutilmente ; itero nel cont
	

cont:	add r0, r0, #1
	b loop

fine:	mov pc, lr

	.data
s:	.string "Ciao mondo"
