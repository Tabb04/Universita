#define _POSIX_C_SOURCE 200809L // Enable POSIX features for sigaction
#include <stdio.h>
#include <stdlib.h> 
#include <signal.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>

void gestore_segnali(int sig){
    printf("Ricevuto segnale %d, continuo\n", sig);
    printf("Premi Ctrl + c per riprovare\n");
    fflush(stdout);
}

//DEVI COMPILARE CON GCC -C NOME NOME.C        --> NON CON COMPILATORE DI VSCODE

int main(){
    //faccio ciclo cattura segnale in loop
    struct sigaction sa;

    printf("Programma %d avviato, Premi Ctrl+c.\n", getpid());
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = gestore_segnali;
    sigemptyset(&sa.sa_mask);       //--> non blocco nessun segnale tranne sigint

    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        perror("Errore in sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Attendo segnali\n");

    while(1){
        pause();
        printf("...pause() è ritornata dopo la gestione del segnale.\n");
    }

    return 0;
}