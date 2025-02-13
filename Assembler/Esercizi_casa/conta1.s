    .text
    .global conta

conta:
    push {lr}             @ Salva il link register
    mov r1, #0            @ r1 conterà le occorrenze (inizialmente 0)

loop:
    ldrb r2, [r0]         @ Carica il carattere corrente in r2
    cmp r2, #0            @ Controlla se siamo alla fine della stringa
    beq done              @ Se sì, esci dal loop

    ldrb r3, [r0, #1]     @ Carica il carattere successivo in r3
    cmp r3, #0            @ Controlla se il successivo è un terminatore
    beq done              @ Se sì, esci dal loop

    add r4, r2, #1        @ Calcola r2 + 1
    cmp r3, r4            @ Confronta r3 con r2 + 1
    addeq r1, r1, #1      @ Se sono uguali, incrementa il contatore in r1

    add r0, r0, #1        @ Passa al carattere successivo
    b loop                @ Ripeti il loop

done:
    mov r0, r1            @ Metti il risultato in r0 (valore di ritorno)
    pop {pc}              @ Ritorna al chiamante

