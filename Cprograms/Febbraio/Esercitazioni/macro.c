// Scriviamo un programma che fa un uso estensivo delle direttive
// di precompilazione. In particolare, il programma deve definire
// una macro MAX che restituisce il massimo tra due numeri interi.

// Il programma deve poi definire una macro che restituisce il
// massimo tra tre numeri interi. Inoltre introduciamo una macro
// per gestire i messaggi di debug. La macro DEBUG deve essere
// definita per abilitare i messaggi di debug. La macro deve
// stampare il messaggio di debug e il file e la linea in cui
// si trova il messaggio. Infine, il programma deve definire una
// macro per la versione del programma. La versione del programma
// deve essere 1 o 2. Se la versione non è 1 o 2, il programma
// deve terminare con un errore. Il programma deve stampare la
// versione del programma e il massimo tra tre numeri interi.

#define MAX (a, b) ((a) > (b) ? (a):(b))
#define MAX2 (a, b, c) (MAX(MAX(a, b)), c)

#ifdef DEBUG