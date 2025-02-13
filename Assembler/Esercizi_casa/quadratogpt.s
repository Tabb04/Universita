.text
.global main

@ Prendo 4 argomenti e calcolo a*x^2 + b*x + c
main:   
    push {lr, r4, r5, r6}    @ Salva i registri necessari

    ldr r0, [r1, #4]         @ argv[1] -> a
    bl atoi
    mov r2, r0               @ Salva a in r2

    ldr r0, [r1, #8]         @ argv[2] -> b
    bl atoi
    mov r3, r0               @ Salva b in r3

    ldr r0, [r1, #12]        @ argv[3] -> c
    bl atoi
    mov r4, r0               @ Salva c in r4

    ldr r0, [r1, #16]        @ argv[4] -> x
    bl atoi
    mov r5, r0               @ Salva x in r5

    mul r6, r5, r5           @ r6 = x^2
    mul r2, r2, r6           @ r2 = a * x^2

    mul r3, r3, r5           @ r3 = b * x
    add r2, r2, r3           @ r2 = a*x^2 + b*x

    add r2, r2, r4           @ r2 = a*x^2 + b*x + c

    ldr r0, =v               @ Carica il puntatore alla stringa "Risultato e' %d"
    mov r1, r2               @ Passa il risultato come secondo argomento
    bl printf                @ Chiama printf

    pop {lr, r4, r5, r6}     @ Ripristina i registri
    mov pc, lr               @ Ritorna al chiamante

.data
v:  .string "Risultato e' %d\n"  @ Stringa formattata

