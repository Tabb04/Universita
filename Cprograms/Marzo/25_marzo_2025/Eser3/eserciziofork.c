#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h> // per fork e getpid
#include "Syscalls.h"
#define VERSION2

/*
La funzione fork() restituisce un valore diverso a seconda del contesto:
- Nel processo padre: restituisce il PID (Process ID) del processo figlio.
- Nel processo figlio: restituisce 0.
- In caso di errore: restituisce -1.
*/

int padre(int pid){
    printf("Processo padre, PID: %d, PID_FIGLIO: %d\n", getpid(), pid);
}

int figlio(){
    printf("Processo figlio, PID: %d\n", getpid());
}



int main(){
    pid_t pid1;
    
    #ifdef VERSION2
    SCALL(pid1, fork(), "Errore durante fork");
    PARENT_OR_CHILD(pid1, padre(pid1), figlio());
    #else
    if ((pid1 = fork()) == -1){
        perror("Errore fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0){
        printf("Processo figlio (PID: %d)\n", getpid());
    }else{
        printf("Processo padre (PID: %d, Figlio PID: %d)\n", getpid(), pid1);
    }
    #endif
    
    return 0;
}