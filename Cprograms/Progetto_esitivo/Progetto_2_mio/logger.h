#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h> // Per FILE*
#include <stdbool.h>

// Eventuali tipi di evento di log, come richiesto dalla specifica
typedef enum {
    LOG_EVENT_FILE_PARSING,
    LOG_EVENT_MESSAGE_QUEUE,
    LOG_EVENT_EMERGENCY_STATUS,
    LOG_EVENT_RESCUER_STATUS,
    LOG_EVENT_ASSIGNMENT,
    LOG_EVENT_TIMEOUT,
    LOG_EVENT_GENERAL_INFO,
    LOG_EVENT_GENERAL_ERROR
    // Aggiungere altri tipi se necessario
} log_event_type_t;

/**
 * @brief Inizializza il logger. Deve essere chiamata una volta all'inizio.
 * @param log_filename Il nome del file dove scrivere i log.
 * @return true se l'inizializzazione ha successo, false altrimenti.
 */
bool init_logger(const char *log_filename);

/**
 * @brief Chiude il logger e rilascia le risorse.
 */
void close_logger(void);

/**
 * @brief Scrive un messaggio di log nel file.
 *
 * @param event_type Il tipo di evento di log.
 * @param id L'ID associato all'evento (es. ID emergenza, ID file, "N/A"). Max 32 caratteri.
 * @param message Il messaggio di log da scrivere.
 */
void log_message(log_event_type_t event_type, const char *id, const char *message);

// Funzione di utilità per convertire log_event_type_t in stringa
const char* log_event_type_to_string(log_event_type_t event_type);

#endif // LOGGER_H