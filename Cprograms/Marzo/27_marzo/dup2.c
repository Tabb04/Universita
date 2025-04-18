#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "Syscalls.h"

int fd[2];

int figlio(){
    close(fd[0]);                       //chiudo lettura
    dup2(fd[1], STDOUT_FILENO);         //collego stdout e scrittura pipe
    close(fd[1]);
    execlp("ls", "ls", NULL);
    perror("Errore exec");
    exit(EXIT_FAILURE);
}

int padre(){
    close(fd[1]);                       //chiudo scrittura
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    printf("Ci sono tot file: ");
    fflush(stdout);
    execlp("wc", "wc", "-l", NULL);
    perror("Errore exec");
    exit(EXIT_FAILURE);
}




int main(){
    int res;
    SCALL(res, pipe(fd), "Errore in pipe");

    pid_t pid1;
    SCALL(pid1, fork(), "Errore fork");

    PARENT_OR_CHILD(pid1, padre(), figlio());
    return 0;

}