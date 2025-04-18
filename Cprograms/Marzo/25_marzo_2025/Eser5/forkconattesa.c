#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "Syscalls.h"

int main(){
    pid_t pid1; 

    if ((pid1 = fork()) == -1){
        perror("Erorre in fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0){
        printf("Processo figlio, PID: %d, sto eseguendo ls -l\n", getpid());
        char* argomenti[] = {"/bin/ls", "-l", NULL};
        execvp(argomenti[0], argomenti);
        //execlp("ls", "ls", "-l", NULL);
        perror("Errore durante execvp");
        exit(EXIT_FAILURE);
    }else{
        pid_t pid2;
        int stato;
        if ((pid2 = waitpid(pid1, &stato, 0)) == -1){ //restituisce pid del figlio terminato
            perror("Errore durante waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(stato)){
            if (pid1 == pid2){
                printf("I due pid corrispondono correttamente!\n");
            }
            printf("Figlio uscito correttamente, con stato %d\n", WEXITSTATUS(stato));
        }
    }
    return 0;
}