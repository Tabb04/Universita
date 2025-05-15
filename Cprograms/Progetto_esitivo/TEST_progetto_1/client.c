#define _POSIX_C_SOURCE 200809L 
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

#include "data_types.h" 
#include "config1.h"   

#define MATRICOLA "674556"
#define MQ_BASE_NAME "/emergenze"


bool send_emergency_request(mqd_t mqdes, const char *emergency_name, int x, int y, int delay_secs) {
    emergency_request_t request;
    char mq_log_buffer[256]; // Buffer per messaggi interni al client

    if (strlen(emergency_name) >= EMERGENCY_NAME_LENGTH) {
        fprintf(stderr, "Errore: Nome emergenza '%s' troppo lungo (max %d caratteri).\n",
                emergency_name, EMERGENCY_NAME_LENGTH - 1);
        return false;
    }
    strncpy(request.emergency_name, emergency_name, EMERGENCY_NAME_LENGTH -1);
    request.emergency_name[EMERGENCY_NAME_LENGTH -1] = '\0';

    request.x = x;
    request.y = y;
    // Interpretiamo delay_secs come un ritardo prima dell'invio.
    // Il timestamp sarà il momento dell'effettivo "accadimento" o registrazione,
    // che per semplicità impostiamo a time(NULL) al momento della costruzione della richiesta,
    // *prima* dell'eventuale delay del client.
    // Se delay_secs dovesse influenzare il timestamp dell'evento, la logica qui cambierebbe.
    request.timestamp = time(NULL);

    if (delay_secs > 0) {
        printf("Client: In attesa per %d secondi prima di inviare l'emergenza '%s'...\n", delay_secs, emergency_name);
        // Usiamo clock_nanosleep per una sleep più precisa e interrompibile
        struct timespec delay_ts;
        delay_ts.tv_sec = delay_secs;
        delay_ts.tv_nsec = 0;
        // Loop per gestire l'interruzione di clock_nanosleep da segnali
        while(clock_nanosleep(CLOCK_MONOTONIC, 0, &delay_ts, &delay_ts) == EINTR);
    }

    printf("Client: Invio emergenza: Tipo='%s', Pos=(%d,%d), Timestamp=%ld\n",
           request.emergency_name, request.x, request.y, (long)request.timestamp);

    if (mq_send(mqdes, (const char *)&request, sizeof(emergency_request_t), 0) == -1) {
        // Usiamo 0 come priorità del messaggio, il server non la usa per mq_receive da specifiche
        snprintf(mq_log_buffer, sizeof(mq_log_buffer), "Errore client: mq_send fallito per emergenza '%s'", request.emergency_name);
        perror(mq_log_buffer);
        return false;
    }

    printf("Client: Emergenza '%s' inviata con successo.\n", request.emergency_name);
    return true;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso:\n");
        fprintf(stderr, "  %s <nome_emergenza> <coord_x> <coord_y> <delay_in_secs>\n", argv[0]);
        fprintf(stderr, "  %s -f <file_richieste>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char mq_name[LINE_LENGTH];
    snprintf(mq_name, sizeof(mq_name), "%s%s", MQ_BASE_NAME, MATRICOLA);

    // Apri la coda di messaggi (solo scrittura)
    // Non creiamo la coda qui, il server dovrebbe averla già creata.
    // Se il server non è attivo, mq_open fallirà (correttamente).
    mqd_t mqdes = mq_open(mq_name, O_WRONLY);
    if (mqdes == (mqd_t)-1) {
        fprintf(stderr, "Errore client: Impossibile aprire la coda di messaggi '%s'. Assicurarsi che il server sia in esecuzione.\n", mq_name);
        perror("Dettaglio errore mq_open");
        return EXIT_FAILURE;
    }
    printf("Client: Coda di messaggi '%s' aperta con successo.\n", mq_name);

    bool success = true; // Flag per tracciare se tutti gli invii hanno avuto successo

    if (strcmp(argv[1], "-f") == 0) {
        // Modalità file
        if (argc != 3) {
            fprintf(stderr, "Errore: Opzione -f richiede un nome file.\n");
            fprintf(stderr, "Uso: %s -f <file_richieste>\n", argv[0]);
            mq_close(mqdes);
            return EXIT_FAILURE;
        }
        const char *filename = argv[2];
        FILE *file = fopen(filename, "r");
        if (!file) {
            fprintf(stderr, "Errore client: Impossibile aprire il file di richieste '%s'.\n", filename);
            perror("Dettaglio errore fopen");
            mq_close(mqdes);
            return EXIT_FAILURE;
        }
        printf("Client: Lettura richieste dal file '%s'...\n", filename);

        char line_buffer[LINE_LENGTH];
        int line_num = 0;
        while (fgets(line_buffer, sizeof(line_buffer), file)) {
            line_num++;
            // Rimuovi newline
            line_buffer[strcspn(line_buffer, "\n")] = 0;

            char name_from_file[EMERGENCY_NAME_LENGTH];
            int x_from_file, y_from_file, delay_from_file;

            // Parsing della riga (molto semplice, si aspetta spazi come separatori)
            // Formato: <nome_emergenza_senza_spazi> <coord_x> <coord_y> <delay_in_secs>
            // Per nomi con spazi, sscanf diventa più complesso.
            // Per ora, assumiamo nomi emergenza senza spazi.
            if (sscanf(line_buffer, "%s %d %d %d", name_from_file, &x_from_file, &y_from_file, &delay_from_file) == 4) {
                if (!send_emergency_request(mqdes, name_from_file, x_from_file, y_from_file, delay_from_file)) {
                    fprintf(stderr, "Client: Errore invio richiesta da file (riga %d): %s\n", line_num, line_buffer);
                    success = false; // Segna che almeno un invio è fallito
                    // Decidere se continuare con le altre righe o interrompere
                    // Per ora continuiamo
                }
            } else {
                fprintf(stderr, "Client: Formato riga %d errato nel file: '%s'. Riga ignorata.\n", line_num, line_buffer);
                success = false; // Considera un errore di formato come un fallimento parziale
            }
        }
        fclose(file);

    } else {
        // Modalità argomenti da riga di comando
        if (argc != 5) {
            fprintf(stderr, "Errore: Numero di argomenti errato per la richiesta singola.\n");
            fprintf(stderr, "Uso: %s <nome_emergenza> <coord_x> <coord_y> <delay_in_secs>\n", argv[0]);
            mq_close(mqdes);
            return EXIT_FAILURE;
        }
        const char *emergency_name_arg = argv[1];
        // atoi può essere problematico se le stringhe non sono numeri validi,
        // per robustezza si dovrebbe usare strtol con controllo errori.
        int x_arg = atoi(argv[2]);
        int y_arg = atoi(argv[3]);
        int delay_arg = atoi(argv[4]);

        if (!send_emergency_request(mqdes, emergency_name_arg, x_arg, y_arg, delay_arg)) {
            success = false;
        }
    }

    // Chiudi la coda di messaggi
    if (mq_close(mqdes) == -1) {
        perror("Errore client: mq_close fallito");
        // Anche se la chiusura fallisce, tentiamo di uscire con lo stato corretto
        // basato sul successo degli invii.
        return success ? EXIT_FAILURE : EXIT_FAILURE; // Sempre FAILURE se mq_close fallisce
    }
    printf("Client: Coda di messaggi chiusa.\n");

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}