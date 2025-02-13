	.global main

main:	mov r1, #1	@indice
	mov r2, #4	@N di check
	mov r0, #0	@sum

loop:	cmp r1, r2
	bge r1, r2
	add r1, r1, #1
	b loop

fine:	mov pc, lr
	
	
	
	
	
	
