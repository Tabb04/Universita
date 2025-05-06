#include "logger.h"
#include <time.h>
#include <string.h> // Per strerror
#include <errno.h>  // Per errno
#include <stdarg.h> // Per va_list, va_start, va_end

static FILE *log_file = NULL;

// Funzione per convertire enum in stringa per il log
const char* log_event_type_to_string(log_event_type_t event_type) {
    switch (event_type) {
        case LOG_EVENT_FILE_PARSING: return "FILE_PARSING";
        case LOG_EVENT_MESSAGE_QUEUE: return "MESSAGE_QUEUE";
        case LOG_EVENT_EMERGENCY_STATUS: return "EMERGENCY_STATUS";
        case LOG_EVENT_RESCUER_STATUS: return "RESCUER_STATUS";
        case LOG_EVENT_ASSIGNMENT: return "ASSIGNMENT";
        case LOG_EVENT_TIMEOUT: return "TIMEOUT";
        case LOG_EVENT_GENERAL_INFO: return "INFO";
        case LOG_EVENT_GENERAL_ERROR: return "ERROR";
        default: return "UNKNOWN_EVENT";
    }
}

bool init_logger(const char *log_filename) {
    if (log_file != NULL) {
        fprintf(stderr, "Logger già inizializzato.\n");
        return false; // Già inizializzato
    }
    // Apre in modalità append ("a") per non sovrascrivere i log precedenti ad ogni avvio.
    // Se preferisci sovrascrivere, usa "w".
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "ERRORE CRITICO: Impossibile aprire il file di log '%s': %s\n", log_filename, strerror(errno));
        return false;
    }
    // Logga l'inizio della sessione di logging
    log_message(LOG_EVENT_GENERAL_INFO, "SYSTEM", "Logger inizializzato.");
    return true;
}

void close_logger(void) {
    if (log_file != NULL) {
        log_message(LOG_EVENT_GENERAL_INFO, "SYSTEM", "Logger in chiusura.");
        fflush(log_file); // Assicura che tutti i buffer siano scritti
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(log_event_type_t event_type, const char *id, const char *message) {
    // 1. Controllo se il file di log è aperto
    if (log_file == NULL) {
        // Fallback a stderr se il file di log non è aperto (es. init_logger fallita o non chiamata)
        fprintf(stderr, "FALLBACK LOGGER (file non aperto): [%s] [%s] %s\n",
                log_event_type_to_string(event_type), // Converte l'enum in una stringa leggibile
                (id ? id : "N/A"),                    // Se 'id' è NULL, usa "N/A", altrimenti usa 'id'
                message);                             // Il messaggio effettivo
        return; // Esce dalla funzione se non si può scrivere su file
    }

    // 2. Ottenere il Timestamp attuale
    time_t current_time = time(NULL); // time_t è solitamente un tipo long (o simile)
                                      // time(NULL) restituisce il numero di secondi trascorsi
                                      // dall'Epoch (00:00:00 UTC, 1 Gennaio 1970),
                                      // o -1 in caso di errore (anche se -1 è un valore long valido).

    // 3. Scrittura del messaggio formattato nel file di log
    // Formato Specificato: [Timestamp] (ID) [Evento] Messaggio.
    fprintf(log_file, "[%ld] (%s) [%s] %s\n",
            (long)current_time,                     // Timestamp come numero intero lungo (long)
            (id ? id : "N/A"),                      // Gestione dell'ID nullo, come sopra
            log_event_type_to_string(event_type),   // Stringa dell'evento
            message);                               // Il messaggio

    // 4. Svuotamento del buffer (Flush)
    fflush(log_file); // Forza la scrittura immediata del contenuto del buffer del file di log
                      // sul disco. Utile per il debug per vedere i log subito,
                      // ma può avere un impatto sulle prestazioni se fatto troppo spesso
                      // in applicazioni ad alta intensità di logging.
                      // Per produzione, si potrebbe rimuovere o rendere condizionale.
}


/*
void log_message(log_event_type_t event_type, const char *id, const char *message) {
    if (log_file == NULL) {
        // Fallback a stderr se il file di log non è aperto
        fprintf(stderr, "FALLBACK LOGGER (file non aperto): [%s] [%s] %s\n",
                log_event_type_to_string(event_type),
                (id ? id : "N/A"),
                message);
        return;
    }

    time_t current_time = time(NULL); // Timestamp in secondi (long)
    // Nota: time(NULL) restituisce -1 in caso di errore, che come long è valido.
    // Per una formattazione più leggibile del tempo, si potrebbe usare strftime.

    // Formato: [Timestamp] (ID) [Evento] Messaggio.
    fprintf(log_file, "[%ld] (%s) [%s] %s\n",
            (long)current_time,
            (id ? id : "N/A"), // Usa "N/A" se l'ID è nullo
            log_event_type_to_string(event_type),
            message);
    fflush(log_file); // Scrive immediatamente, utile per il debug, ma può impattare le performance.
                      // Rimuovere o rendere condizionale per produzione.
}
*/