#include "logger.h"
#include <time.h>
#include <string.h> // Per strerror
#include <errno.h>  // Per errno
#include <stdarg.h> // Per va_list, va_start, va_end
#include "Syscalls_3_progetto.h"

static FILE* log_file = NULL;

const char* log_event_type_to_string(log_event_type_t event_type){
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

bool init_logger(const char* log_filename){
    if(log_file != NULL){
        perror("File già inizializzato");
        return false;
    }

    //qui posso proteggere con le solite macro perchè non ho un log_file dove scrivere
    E_SNCALL(log_file, fopen(log_filename, "a"), "Errore fopen");

    log_message(LOG_EVENT_GENERAL_INFO, "SYSTEM", "Logger inizializzato");
    return true;
}

void close_logger(){
    if(log_file != NULL){
        log_message(LOG_EVENT_GENERAL_INFO, "SYSTEM", "Logger in chiusura");
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(log_event_type_t event_type, const char* id, const char* message){
    if(log_file == NULL){
        perror("File di log non è stato aperto, non posso loggare questo errore");
        return;
    }
    time_t tempo_attuale = time(NULL);

    fprintf(log_file, "[%ld] (%s) [%s] %s\n", (long)tempo_attuale, (id ? id: "NULL"), log_event_type_to_string(event_type), message);
    fflush(log_file);
}