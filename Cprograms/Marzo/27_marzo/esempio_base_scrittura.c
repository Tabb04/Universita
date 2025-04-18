#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "Syscalls.h"

int fd[2]; 


int figlio(){
    close(fd[1]);       //chiudo lato scrittura
    char buffer[100];
    read(fd[0], buffer, sizeof(buffer));
    printf("Figlio ha letto: %s\n", buffer);
    close(fd[0]);
}

int padre(){
    close(fd[0]);       //chiudo lato lettura
    char msg[] = "Ciao figlio!";
    write(fd[1], msg, strlen(msg) + 1);
    close(fd[1]);
}


int main(){

    int test;     //fd[0] è lettura, fd[1] è scrittura
    SCALL(test, pipe(fd), "Errore in pipe");
    
    pid_t pid1;
    SCALL(pid1, fork(), "Errore in fork");

    PARENT_OR_CHILD(pid1, padre(), figlio());

    return 0;
}