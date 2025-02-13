    .text
    .global main

main:
    push {lr}                   @ Salva il link register
    ldr r0, [r1, #4]            @ Carica argv[1] in r0 (primo argomento passato)
    bl atoi                     @ Converte argv[1] in un intero (n)
    
    bl fact                     @ Calcola il fattoriale di n


fact:
    cmp r0, #1                  @ Caso base: se n <= 1, ritorna 1
    moveq r0, #1
    moveq pc, lr

    push {lr}                   @ Salva il link register
    push {r0}                   @ Salva n
    sub r0, r0, #1              @ n = n - 1
    bl fact                     @ Chiama fact(n-1)

    pop {r1}                    @ Ripristina n originale da stack
    mul r0, r0, r1              @ Moltiplica n * fact(n-1)
    pop {pc}                    @ Ripristina lr e ritorna

