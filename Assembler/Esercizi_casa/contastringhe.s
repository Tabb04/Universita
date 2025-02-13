    .text
    .global conta_stringhe

conta_stringhe:
    push {lr}                @ Salva il link register
    mov r2, #0               @ r2 sarà il contatore totale delle occorrenze (inizializzato a 0)

loop:
    cmp r1, #0               @ Controlla se sono rimaste stringhe da processare
    beq done                 @ Se n == 0, termina

    ldr r3, [r0], #4         @ Carica l'indirizzo della stringa corrente (v[i]) in r3 e incrementa r0
    mov r0, r3               @ Sposta l'indirizzo della stringa in r0 per la funzione `conta`
    bl conta                 @ Chiamata alla funzione conta

    add r2, r2, r0           @ Aggiungi il risultato della conta alla somma totale
    sub r1, r1, #1           @ Decrementa il contatore delle stringhe rimanenti
    b loop                   @ Ripeti il ciclo

done:
    mov r0, r2               @ Restituisci il risultato finale in r0
    pop {pc}                 @ Ritorna al chiamante

