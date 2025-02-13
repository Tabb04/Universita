			.text
			.global lunghezza_lista
			.type lunghezza_lista, %function

lunghezza_lista:
 			push {lr}
 			mov r1, #0		@lunghezza

loop:
    			cmp r0, #0              @se nodo = 0, vuol dire NULL in teoria
    			beq fine                 
    			add r1, r1, #1          @lunghezza + 1
    			ldr r0, [r0, #4]        
   			b loop                  

fine:
    			mov r0, r1
    			pop {pc}

