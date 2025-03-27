#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <Syscall.h>

#define BUFFER_SIZE 128
#define FILENAME "test.txt"


/*
Il costrutto do { ... } while(0) è usato per garantire che la macro si comporti correttamente
in tutti i contesti, specialmente quando viene utilizzata in un blocco condizionale
senza parentesi graffe
*/

/*
#define SCALL(r, c, e) do {if((r = c) == -1) {perror(e); exit(EXIT_FAILURE); }} while(0)
#define SCALLREAD(a, b, c, d) \
    do{ \
        while((a = b) > 0){ \
            c; \
        }\
        if (a == -1){ \
            perror(d);\
            exit(EXIT_FAILURE);\
        }\
    }while(0)

*/
#define VERSION2
//#undef VERSION2

int main(){
    
    int descrittore;
    char buffer[BUFFER_SIZE];
    __ssize_t byte_letti;

    #ifdef VERSION2
        SCALL(descrittore, open(FILENAME, O_RDONLY), "Errore durante open");
        SCALLREAD(byte_letti, read(descrittore, buffer, BUFFER_SIZE), write(STDOUT_FILENO, buffer, byte_letti), "Errore durante read");
        SCALL(descrittore, close(descrittore), "Errore durante close");    
    #else
        descrittore = open(FILENAME, O_RDONLY);
        if (descrittore == -1){
            perror("Durante open");
            exit(EXIT_FAILURE);
        }

        while((byte_letti = read(descrittore, buffer, BUFFER_SIZE)) > 0){
            write(STDOUT_FILENO, buffer, byte_letti);
        }
    
        if (byte_letti == -1){
            perror("Errore nella read");
        }

        close(descrittore);

    #endif
    return 0;
}