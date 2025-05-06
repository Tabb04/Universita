#define _POSIX_C_SOURCE 200809L // Enable POSIX features for sigaction

#include <stdio.h>
#include <stdlib.h>     
#include <signal.h>     
#include <unistd.h>     
#include <string.h>     
#include <time.h>       
#include <errno.h>
#include <fcntl.h>

#include "Syscalls_1.h"
const char *nome_file_log = "activities.log";
#define DIM_BUFFER 256


volatile sig_atomic_t richiesta_terminazione = 0;
//volatile e sig_atomic servono quando si usano segnali
int fd = -1;
char buffer[DIM_BUFFER];
int ris = 0;


void gestore_segnali(int sig_num){
    //potrei stampare sig_num come int per dire che numero di segnale è
    richiesta_terminazione = 1;
    //non posso usare printf
    //uso funzione async-signal-safe
    const char msg[] = "\nCtrl+c premuto, terminazione avviata\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1); //posso usare strlen?
}

void pulizia(){
    printf("Avvio cleanup risorse\n");
    if (fd != -1){
        printf("Chiusura del file %s\n", nome_file_log);
        snprintf(buffer, sizeof(buffer), "[%ld] Chiusura del file dopo SIGINT\n", (long)time(NULL));
        write(fd, buffer, strlen(buffer));
        SCALL(ris, close(fd), "Errore close");

    }else{
        printf("Nessun file di Log da chiudere\n");
    }
    
    printf("Cleanup completato. Uscita\n");
    exit(EXIT_SUCCESS);
}


int main(){
    struct sigaction sa;
    time_t tempo_corrente;
    int counter = 0;

    printf("Programma avviato con PID: %d\n", getpid());
    printf("Attività registrare nel file %s\n", nome_file_log);
    printf("Ctrl+c per terminazione pulita\n");

    SCALL(fd, open(nome_file_log, O_WRONLY | O_CREAT | O_APPEND, 0644), "Errore Open");
    tempo_corrente = time(NULL);
    
    snprintf(buffer, sizeof(buffer), "--- Sessione di Log iniziata[%ld] ---\n", (long)tempo_corrente);
    write(fd, buffer, strlen(buffer));
    //fflush(fd);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore_segnali;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;   //->riavvia chiamate di sistema lente (come sleep) se interrotte

    SCALL(ris, sigaction(SIGINT, &sa, NULL), "Errore sigaction");
    //non ho bisogno di controllare se ha fallito per chiudere
    //i descrittori di file perchè lo fa la exit

    printf("Sigaction inizializzato, inzio programma\n");

    while(richiesta_terminazione == 0){
        tempo_corrente = time(NULL);
        snprintf(buffer, sizeof(buffer), "[%ld] Loop %d - Tutto ok. %s", (long)tempo_corrente, counter++, ctime(&tempo_corrente));
        write(fd, buffer, strlen(buffer));
        //fflush(fd);

        printf("Ciclo %d completato. In attesa... (Premi Ctrl+c per uscire)\n", counter);

        //aspetto e se catturo segnale il gestore imposta richiesta_terminazione a 1
        //quando torna in base a come è impostata la flag termina o restarta la sleep
        sleep(5);
    }

    printf("Terminazine rilevata\n");
    pulizia();
    
    //mai raggiunto perchè faccio exit
    return 0;
}