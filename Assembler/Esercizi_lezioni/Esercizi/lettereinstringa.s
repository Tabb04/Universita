	.text
	.global main

main:	push {lr}
	ldr r0, =s	@r0 il vettore
	mov r1, #'c'	@lettera da cercare
	mov r2, #0	@r2 totale
	
loop:	ldrb r3, [r0]	@in r1 la prima lettera
	cmp r3, #0
	beq fine
	cmp r3, r1
	addeq r2, r2, #1
	add r0, r0, #1
	b loop

fine:	ldr r0, =v
	bl printf
	pop {pc}



	.data
s:	.string "Banana"
v:	.string "La lettera %c compare %d volte"
