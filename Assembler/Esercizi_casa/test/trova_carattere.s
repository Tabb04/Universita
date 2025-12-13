	@Funzione che dato un array di caratteri e un carattere di input, restituisce
	@la posizione della prima ricorrenza del carattere


	.text
	.global main
	
main:
	push {r4}
	mov r4, #'e'	@lettera da trovare
	mov r2, #9	@dimensione array
	mov r3, #0	@counter
	ldr r0, =v

loop:
	ldrb r1, [r0, r3]
	cmp r1, r4
	beq trovato
	cmp r3, r2
	bge non_trovato
	add r3, r3, #1
	b loop

trovato:
	mov r0, r3
	b fine

non_trovato:
	mov r0, #-1
	
fine:
	pop {r4}
	mov pc, lr	


	.data
v:	.string "abcdefghij"
