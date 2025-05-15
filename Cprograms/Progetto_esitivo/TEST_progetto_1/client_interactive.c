// client_interactive.c
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE // Per strsignal, sebbene non strettamente necessario per la logica base

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h> // Per la gestione di SIGINT

// Includi le definizioni delle strutture dati e costanti
#include "data_types.h"
#include "config1.h"

#define MATRICOLA "674556" // <<< MODIFICA CON LA MATRICOLA USATA DAL SERVER
#define MQ_BASE_NAME "/emergenze"
#define INPUT_BUFFER_SIZE (EMERGENCY_NAME_LENGTH + 50) // Spazio per nome, coordinate, delay e spazi

// Flag globale per SIGINT
volatile sig_atomic_t sigint_received_client = 0;

void client_sigint_handler(int signo) {
    if (signo == SIGINT) {
        sigint_received_client = 1;
        // Scrivere qui può essere rischioso, ma per un client è meno critico che per un server.
        // Un approccio più sicuro sarebbe non scrivere nulla e lasciare che il loop principale lo gestisca.
        // write(STDOUT_FILENO, "\nCtrl+C ricevuto dal client. Uscita...\n", strlen("\nCtrl+C ricevuto dal client. Uscita...\n"));
    }
}

// Funzione per inviare una singola richiesta (leggermente modificata per non fare sleep)
// La sleep la gestirà il chiamante se necessario.
bool send_emergency_request_interactive(mqd_t mqdes, const char *emergency_name, int x, int y) {
    emergency_request_t request;
    char mq_log_buffer[256];

    if (strlen(emergency_name) >= EMERGENCY_NAME_LENGTH) {
        fprintf(stderr, "Client Errore: Nome emergenza '%s' troppo lungo (max %d caratteri).\n",
                emergency_name, EMERGENCY_NAME_LENGTH - 1);
        return false;
    }
    strncpy(request.emergency_name, emergency_name, EMERGENCY_NAME_LENGTH -1);
    request.emergency_name[EMERGENCY_NAME_LENGTH -1] = '\0';

    request.x = x;
    request.y = y;
    request.timestamp = time(NULL); // Timestamp al momento dell'invio

    printf("Client: Invio emergenza: Tipo='%s', Pos=(%d,%d), Timestamp=%ld\n",
           request.emergency_name, request.x, request.y, (long)request.timestamp);

    if (mq_send(mqdes, (const char *)&request, sizeof(emergency_request_t), 0) == -1) {
        snprintf(mq_log_buffer, sizeof(mq_log_buffer), "Client Errore: mq_send fallito per emergenza '%s'", request.emergency_name);
        perror(mq_log_buffer);
        return false;
    }

    printf("Client: Emergenza '%s' inviata con successo.\n", request.emergency_name);
    return true;
}

int main(void) { // Non usiamo argc, argv per questo client interattivo
    struct sigaction sa_client_int;
    char mq_name[LINE_LENGTH];
    mqd_t mqdes;
    char input_buf[INPUT_BUFFER_SIZE];
    char emergency_name_input[EMERGENCY_NAME_LENGTH];
    int x_input, y_input, delay_input; // Delay non usato per l'invio diretto, ma letto

    // Configura il gestore per SIGINT (Ctrl+C)
    memset(&sa_client_int, 0, sizeof(sa_client_int));
    sa_client_int.sa_handler = client_sigint_handler;
    if (sigaction(SIGINT, &sa_client_int, NULL) == -1) {
        perror("Client Errore: Impossibile registrare gestore SIGINT");
        return EXIT_FAILURE;
    }

    snprintf(mq_name, sizeof(mq_name), "%s%s", MQ_BASE_NAME, MATRICOLA);

    printf("Client Interattivo: Tentativo di connessione alla coda '%s'...\n", mq_name);
    mqdes = mq_open(mq_name, O_WRONLY);
    if (mqdes == (mqd_t)-1) {
        fprintf(stderr, "Client Errore: Impossibile aprire la coda di messaggi '%s'. Assicurarsi che il server sia in esecuzione.\n", mq_name);
        perror("Dettaglio errore mq_open");
        return EXIT_FAILURE;
    }
    printf("Client: Coda di messaggi '%s' aperta. Inserisci le emergenze o Ctrl+C per uscire.\n", mq_name);
    printf("Formato: <NomeEmergenza> <X> <Y> [<DelaySecs> - il delay non verrà usato, invio immediato]\n");
    printf("Esempio: Incendio 10 20 0\n");
    printf("Prompt > ");

    while (!sigint_received_client) {
        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            if (feof(stdin)) {
                // EOF (Ctrl+D) ricevuto, consideriamolo come un segnale di uscita
                printf("\nClient: EOF ricevuto. Uscita.\n");
                sigint_received_client = 1; // Forza l'uscita dal loop
                break;
            }
            if (sigint_received_client) { // Se fgets è stato interrotto da SIGINT
                break;
            }
            // Altro errore di fgets
            perror("Client Errore: fgets");
            break; // Esci in caso di errore di lettura
        }

        // Rimuovi newline se presente
        input_buf[strcspn(input_buf, "\n")] = 0;

        // Semplice parsing, si aspetta 3 o 4 valori separati da spazio
        // Per nomi con spazi, questo parsing è troppo semplice.
        int items_scanned = sscanf(input_buf, "%s %d %d %d",
                                   emergency_name_input, &x_input, &y_input, &delay_input);

        if (sigint_received_client) break; // Controlla di nuovo dopo sscanf

        if (items_scanned >= 3) { // Almeno nome, x, y. Il delay è opzionale e ignorato.
            if (!send_emergency_request_interactive(mqdes, emergency_name_input, x_input, y_input)) {
                fprintf(stderr, "Client: Fallito l'invio dell'emergenza '%s'.\n", emergency_name_input);
                // Decidere se uscire o permettere altri tentativi
            }
        } else {
            if (strlen(input_buf) > 0 && !sigint_received_client) { // Non stampare errore se l'input era vuoto o interrotto
                fprintf(stderr, "Client: Formato input non valido: '%s'. Richiesti almeno <Nome> <X> <Y>.\n", input_buf);
            }
        }
        if (!sigint_received_client) { // Non stampare il prompt se stiamo uscendo
             printf("Prompt > ");
        }
    }

    printf("\nClient: Chiusura e terminazione...\n");
    if (mq_close(mqdes) == -1) {
        perror("Client Errore: mq_close fallito");
        return EXIT_FAILURE; // Considera un errore grave
    }
    printf("Client: Coda di messaggi chiusa.\n");

    return EXIT_SUCCESS;
}