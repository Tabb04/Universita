.text
.global main

main:
    @ Salva i registri che modificheremo e che devono essere preservati (r4 e lr)
    push {r4, lr}

    ldr r4, =v          @ r4 = indirizzo base del vettore v (usiamo r4, non r1!)
    mov r0, #0          @ Valore di test per la condizione (proviamo il caso r0 == 0)
    mov r2, #1          @ Indice del vettore da usare (es. usiamo l'indice 1, cioè l'elemento '11')
    
    cmp r0, #0          @ Confronta r0 con 0
    beq uguale          @ Se sono uguali, salta al ramo 'then'
    
    @ Ramo 'else': r2++
    add r2, r2, #1      @ Incrementa il registro dell'indice r2
    b   prepara_stampa  @ Salta alla preparazione della stampa

uguale:
    @ Ramo 'then': v[r2]++
    @ Calcoliamo l'indirizzo usando r4 come base sicura
    ldr r3, [r4, r2, LSL #2] @ r3 = v[r2]
    add r3, r3, #1           @ r3++
    str r3, [r4, r2, LSL #2] @ v[r2] = r3
    
prepara_stampa:
    @ Adesso prepariamo la stampa finale per mostrare i risultati
    ldr r0, =strg       @ r0 = indirizzo della stringa di formato (1° argomento printf)
    
    @ r1 = primo valore per %d (l'indice r2)
    mov r1, r2          @ r1 = valore di r2 (2° argomento printf)
    
    @ r2 = secondo valore per %d (il valore v[r2] aggiornato)
    ldr r2, [r4, r1, LSL #2] @ Carichiamo v[r1] in r2 (usiamo r1 che ora contiene l'indice corretto)
                        @ r2 = valore dall'array (3° argomento printf)
    
    bl printf           @ Chiama la funzione printf

fine:
    @ Uscita standard da un programma
    mov r0, #0          @ Codice di ritorno 0 (tutto ok)
    pop {r4, pc}        @ Ripristina r4 e ritorna (equivalente a pop {lr} e mov pc, lr)

.data
v:      .word 10, 11, 12
strg:   .string "Stato finale -> Indice: %d, Valore in v[Indice]: %d\n"
