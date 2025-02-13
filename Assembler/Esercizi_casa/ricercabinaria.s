text
        .global BinarySearch    @ r0 <- puntatore inizio array
                                @ r1 <- size
                                @ r2 <- intero da trovare
                                @ r0 <- valore di ritorno (posizione dell oggetto o -1)
        .type BinarySearch, %function

BinarySearch:   push {r0, lr}           @ Salva il link register e il puntatore dell'array
Chiamata:       cmp r1, #0              @ Controlla se size == 0
                ble notFound            @ Se size == 0, valore non trovato
                asr r1, #1              @ Aggiorna size
                ldr r3, [r0, r1, lsl #2]@ Carica l elemento centrale: array[mid]
                cmp r2, r3              @ Confronta r2 (valore da cercare) con array[mid]
                beq found               @ Se uguale, valore trovato
                blt left                @ Se r2 < array[mid], cerca nella metà sinistra
                                        @ Cerca nella metà destra
right:          add r0, r0, r1, lsl #2  @ Aggiorna r0 (sposta all inizio della metà destra)
                b Chiamata          @ Chiamata ricorsiva
                                        @ Cerca nella metà sinistra
left:           b Chiamata          @ Chiamata ricorsiva

found:          add r0, r1, lsl #2      @ Ritorna l'indice trovato in r0
                pop {r1, lr}
                sub r0, r0, r1
                asr r0, #2
                mov pc, lr

notFound:       mov r0, #-1             @ Valore non trovato
                pop {r1, lr}
                mov pc, lr
