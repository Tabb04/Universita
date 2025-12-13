	.text
	.global main

	@voglio fare if (r0 == 0) v[r2]++ else r2++
main:
	ldr r1, =v
	mov r0, #0
	mov r2, #1
	
	cmp r0, #0
	beq uguale

	add r2, r2, #1
	b stampa

uguale:	

	ldr r3, [r1, r2, LSL #2]	@r3 = v[r2]
	add r3, r3, #1
	str r3, [r1, r2, LSL #2]	

stampa:
	ldr r0, =strg
	mov r1, r2
	ldr r2, [r1, r2, LSL #2]
	
	bl printf

fine:
	mov r0, r0
	mov pc, lr



	.data
v:	.word 10, 11, 12
strg:	.string "L'indice vale %d, valore in v[r2] vale %d"
