	.text
	.global main
	@printo il numero di parametri e il primo parametro

main:	push {lr}		@mi salvo il lr perche lo sporco con la BL
	ldr r2, [r1, #4]	@non devo usare mov r2, r1 perchè sto lavorando con parametri
	mov r1, r0		@in r0 va argc, ergo il numero di parametri
	ldr r0, =str		@in r0 metto il puntatore della stringa perche primo parametro di printf
	bl printf
	pop {pc}		@vado a mettere il lr salvato nello stack nel pc

	.data
str:	.string	"Il numero di parametri e' %d e il primo e' %s\n"
	
	
	
