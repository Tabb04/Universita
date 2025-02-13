.text
.global main

main:
    		push {lr}               
		push {r7}
		push {r8}
  		ldr r0, =strings        @ indirizzo indice stringhe
		mov r7, r0		@mi salvo indirizzo per la sottrazione
    		mov r8, #4              @ n stringhe 
    		bl lunghezza 
	
		pop {r8}
		pop {r7}
    		pop {pc}                 

lunghezza:	push {r4, r5, r6, lr}   
		mov r4, r0              @ in r4 metto r0
		mov r5, #0              @ indice stringa piu' lunga
		mov r6, #0              @ lunghezza massima da confrontare

loop:
		cmp r8, #0              @ ho finito iterazioni
		beq finito		

		ldr r0, [r4], #4        @ carico stringa in r0 per strlen
		bl strlen               
		cmp r0, r6              @ guardo se la lunghezza e' maggiore della mia in r6
 		bgt maggiore          	
		b continua
maggiore:
		mov r6, r0		@nuova max len
		sub r5, r4, #4		@post incremento aveva incrementato di 4 gia'
		sub r5, r5, r7		@calcolo la differenza
		lsr r5, r5, #2		@diviso 4 pk sono 4 byte
continua:
   		sub r8, r8, #1         @ diminuisco n stringhe
   		b loop                 

finito:
		mov r0, r5              @ indice max in r0
		pop {r4, r5, r6, pc}    

.data
strings:
    .word string1
    .word string2
    .word string3
    .word string4

string1:
    .string "Ciao"
string2:
    .string "Come stai?"
string3:
    .string "Io bene"
string4:
    .string "Che giornataccia dio beduino"

