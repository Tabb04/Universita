#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>


#include "Syscalls.h"
int pipe1[2], pipe2[2];

int figlio(){
    close(pipe1[1]);        //chiude scrittura prima pipe
    close(pipe2[0]);        //chiude lettura seconda pipe

    char buffer[100];
    read(pipe1[0], buffer, sizeof(buffer));
    printf("Figlio legge: %s\n", buffer);
    close(pipe1[0]);

    char risposta[] = "Ciao padre!";
    write(pipe2[1], risposta, strlen(risposta) + 1);
    close(pipe2[1]);
}

int padre(){
    close(pipe1[0]);        //chiudo lettura prima pipe
    close(pipe2[1]);        //chiudo scrittura seconda pipe

    char messaggio[] = "Ciao figlio mio!";
    write(pipe1[1], messaggio, strlen(messaggio) + 1);
    close(pipe1[1]);

    char buffer1[100];
    read(pipe2[0], buffer1, sizeof(buffer1));
    printf("Padre legge: %s\n", buffer1);
    close(pipe2[0]);
}



int main(){
    int res;
    SCALL(res, pipe(pipe1), "Errrore in pipe");
    SCALL(res, pipe(pipe2), "Errrore in pipe");

    pid_t pid1;
    SCALL(pid1, fork(), "Errore in fork");
    PARENT_OR_CHILD(pid1, padre(), figlio());

    return 0;
}