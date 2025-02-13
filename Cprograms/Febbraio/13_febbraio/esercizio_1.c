/*
scrivo un programma che usa direttive di precompilazione
definisco una macro Max ...
*/

#include <stdio.h>

#define DEBUG
#define VERSION 2

#define MAX(a, b) ((a) > (b) ? (a):(b)) //restituisce maggiore
//posso usare macro dentro macro?
//si ma non posso farla ricorsiva

#define MAX3 (a, b, c) MAX(MAX((a), (b)), (c))

#ifdef DEBUG
#define DEBUG_MSG(msg) printf("Debug: %s at %s:%d\n", msg, __FILE__, __LINE__ )

#else //se non definito debug viene buttato fuori dal precompilatore
#define DEBUG_MSG(msg) (void* (0)) 
#endif


int main(){

    DEBUG_MSG("Debug mode.\n");

#if VERSION == 1
    printf("Versione 1\n");
#elif VERSION == 2
    printf("Versione 2\n");
#else
    #error "Non supportato"
#endif

    DEBUG_MSG("Controlli...");

    printf("Max of 5, 6, 7: %d", MAX3(5, 6, 7)); //??

    return 0
}