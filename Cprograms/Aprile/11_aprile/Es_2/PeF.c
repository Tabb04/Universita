#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h> // fork e getpid

#include "Syscalls_3.h"
int res;

void padre(pid_t pid1){
    printf("Padre in esecuzione\n");
    SCALL(res, waitpid(pid1, NULL, 0), "Errore in waitpid");
    printf("Processo padre terminato\n");
}

void figlio(){
    printf("Figlio in esecuzione\n");
    char *args[] = {"/bin/ls", "-l", NULL};
    execvp(args[0], args);
    perror("Errore exec");
}

int main(){
    pid_t pid;

    SCALL(pid, fork(), "Errore nella fork");

    PARENT_OR_CHILD(pid, padre, figlio);

    return 0;
}