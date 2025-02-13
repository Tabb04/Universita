        .text
        .global main
	@commenti per input che non funziona
main:   @push {lr}
        @ldr r0, [r1, #4]
        @bl atoi
	mov r0, #4
	bl fact			
	pop {pc}		

fact:   cmp r0, #1              @controllo se sono nel caso base
	bne ric
        mov pc, lr

ric:    push {r0, lr}
        sub r0, r0, #1          @indice per fact(n-1)
        bl fact

        pop {r1, lr}            @in r1 ho messo l'r0 iniziale che sarebbe n
        mul r0, r1, r0          @in r0 risultato di fact(n-1)
        mov pc, lr		@metto pc in lr

