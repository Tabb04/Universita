//macro, makefile e debugging

//debuggare con breakpoint ecc
//scovare errori di memoria


//MACRO: sostituzione lessicale, si identifica con #define


//svolge una routine
//durante preprocessing il nome della macro viene rimpiazzato con il suo valore
//serve per definire costanti
//es #define N 100  --> sostutisce 100 ad ogni N che trova
//si può togliere con #undef

//Esistono macro con parametri
//#define MAX(A,B) if (A > B) printf("%d", A); else printf("%d", B);

//Parametri utili # e ##
//es. #define CONC(A, B) A##B --> su x,y diventa xy, concatena
//es. #define STAMPA(A) printf(#A) --> STAMPA(Ciao!) = printf("Ciao!"), mette virgolette

//Compilazione:
//codice C -> codice C processato ->(compilazione) modulo oggetto ->(linking) eseguibile che usa heap, stack, dati statici...

//Posso preaprare le fasi separate:
//Preprocessing gcc -E codice.c -o codiceP.c
//Compilazione gcc -c codiceP.c -o codice.o
//linking gcc codice.o -o eseguibile.out

//Si può fare compilazione separata per un programma
//es. gcc -c main.c -o main.o,  gcc -c List.c -o List.o,  gcc main.o List.o -o prog
//gcc main.o List.c -o prog
//con grandi programmi è utile se si cambiano solo alcune parti

//Makefile:
//makefiletutorial.com, permette di definire un grado di dipendenze e regole per crare dei file
//non compila e basta ma fa molti comandi come copia, sposta ecc...
//creo file makefile -> includo regole e comandi -> eseguo con make

//Struttura makefile:
//Target list: Dependency list
//             Command 1
//             .
//             .
//             Command n

myProg: main.c List.c Stack.c List.h //posso scrivere tutto il path qua
    gcc main.c List.c -o myProg

//il comando make esegue la prima regola o le regole specificate seguendo il grafico di dipendenze
//Myprog     -> List.o -> List.c
//                     -> List.h
//           -> main.o -> ..
//                     -> List.c


myProg: main.c List.c Stack.c List.h
    gcc main.c List.c -o myProg

main.o: main.c List.h
    gcc -c main.c -o main.o

List.o: List.c List.h
    gcc -c List.c -o List.o

//Si possono utlizzare variabili bash dentro i comandi

objects = main.o List.o

myProg: $(objects)
    gcc $(objects) -o myProg

// ...

//Utilizzo di variabili:
//CC compilatore C da utilizzare
//CFLAGS opzioni di compilazione

CC = gcc
CFLAGS = -Wall -pedantic //termina ai warning

myProg: $(objects)
    $(CC) $(CFLAGS) -c main.c -o main.o

//Si possono anche usare regole implicite come $^ e $@

//Le regole make vengono eseguite solo se i file target non esistono  o non sono più nuovi di..?
//es. target clan,  cancella tutti i file prodotti durante la compilazione

clean: 
    rm *.o
    rm myProg

//si possono anche scrivere brevi test

test: myProg
    echo running test 
    //..?



//DEBUGGING

//es. errore di segmentazione se accedo ad una zona di memoria che non dovevo accedere

//Assert:
//macro per assicurarci che lo stato del programma sia corretto
//assert.h ,   void assert(int espressione) se è vero continua, ..?

//es. int main ... inserisci numero positivo &x
//assert(x>0) --> si interrompe se negativo

//GDB e valgrind:
//gdb esegue il programma passo per passo
//valgrind analizza la memoria allocata


//guarda esempio debug