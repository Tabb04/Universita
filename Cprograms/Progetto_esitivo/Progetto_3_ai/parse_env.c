#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <stdbool.h>
#include <ctype.h> // Per isspace
#include <errno.h>


// Funzione trim_whitespace (uguale a prima)
static void trim_whitespace(char *str) {
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

bool parse_environment(const char *filename, environment_t *env_config) {
    
    char log_msg_buffer[MAX_LINE_LENGTH + 200]; //buffer per messaggi di log formattati

    if (!filename || !env_config) {
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o env_config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_environment", log_msg_buffer);
        //non scrivo perror tanto salvo su file di log
        return false;
    }

    //ci mettto anche il tentativo di apertura file
    sprintf(log_msg_buffer, "Tentativo apertura file: %s", filename);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);



    FILE *file = fopen(filename, "r");

    if (!file) {
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        // perror("Errore apertura file environment"); // Sostituito da log
        // fprintf(stderr, "Impossibile aprire il file di configurazione ambiente: %s\n", filename); // Sostituito da log
        return false;
    }
    
    //qui potrei proteggere con una macro ma devo capire come fare
    //non posso fare exit perchè la gestione la faccio fare al chiamante se ritorno false
    //potrei fare una macro come la SNCALL ma che non fa exit
    //però ho bisogno anche di scrivere su file di log

    //usando la macro sarebbe LOG_SCALL, leggi in Syscalls_3_progetto.h perchè non torna


    log_message(LOG_EVENT_FILE_PARSING, filename, "File aperto con successo.");

    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    bool queue_found = false;
    bool height_found = false;
    bool width_found = false;

    env_config->queue_name[0] = '\0';
    env_config->grid_height = -1;
    env_config->grid_width = -1;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        char original_line_for_log[MAX_LINE_LENGTH]; // Conserva la linea originale per il log
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH -1);
        original_line_for_log[MAX_LINE_LENGTH-1] = '\0';

        if (strchr(original_line_for_log, '\n')) *strchr(original_line_for_log, '\n') = '\0'; // Rimuovi solo per il log

        line[strcspn(line, "\n")] = 0;

        char *trimmed_line = line;
        trim_whitespace(trimmed_line);

        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#') {
            continue;
        }

        char *key = strtok(trimmed_line, "=");
        char *value = strtok(NULL, "=");

        if (key && value) {
            trim_whitespace(key);
            trim_whitespace(value);

            if (strcmp(key, "queue") == 0) {
                strncpy(env_config->queue_name, value, sizeof(env_config->queue_name) - 1);
                env_config->queue_name[sizeof(env_config->queue_name) - 1] = '\0';
                queue_found = true;
                sprintf(log_msg_buffer, "Riga %d: Estratto 'queue' = '%s'", line_num, env_config->queue_name);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

            } else if (strcmp(key, "height") == 0) {
                if (sscanf(value, "%d", &env_config->grid_height) != 1 || env_config->grid_height <= 0) {
                    sprintf(log_msg_buffer, "Riga %d: Errore formato per 'height', valore '%s' non valido.", line_num, value);
                    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                    fclose(file);
                    return false;
                }
                height_found = true;
                sprintf(log_msg_buffer, "Riga %d: Estratto 'height' = %d", line_num, env_config->grid_height);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

            } else if (strcmp(key, "width") == 0) {
                 if (sscanf(value, "%d", &env_config->grid_width) != 1 || env_config->grid_width <= 0) {
                    sprintf(log_msg_buffer, "Riga %d: Errore formato per 'width', valore '%s' non valido.", line_num, value);
                    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                    fclose(file);
                    return false;
                }
                width_found = true;
                sprintf(log_msg_buffer, "Riga %d: Estratto 'width' = %d", line_num, env_config->grid_width);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            } else {
                sprintf(log_msg_buffer, "Riga %d: Chiave sconosciuta '%s' con valore '%s'. Ignorata.", line_num, key, value);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            }
        } else {
            sprintf(log_msg_buffer, "Riga %d: Formato non valido (non chiave=valore): '%s'", line_num, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            // Si potrebbe decidere di fallire qui: fclose(file); return false;
        }
    }

    fclose(file);

    if(!queue_found || !height_found || !width_found) {
        fprintf(stderr, "Errore parsing environment: Uno o più parametri obbligatori (queue, height, width) mancanti nel file %s.\n", filename);
        return false;
    }
    if(strlen(env_config->queue_name) == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Errore: Parametri obbligatori (queue, height, width) mancanti o non validi.");
        return false;
    }

    sprintf(log_msg_buffer, "Completamento parsing: Queue='%s', Height=%d, Width=%d.",
        env_config->queue_name, env_config->grid_height, env_config->grid_width);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    // printf("Configurazione ambiente letta:\n"); // Sostituito da log
    // printf("  Queue: %s\n", env_config->queue_name);
    // printf("  Height: %d\n", env_config->grid_height);
    // printf("  Width: %d\n", env_config->grid_width);
    
    return true;
}


//-------------------------------------------------------------------------



void free_system_config(system_config_t *config) {
    if (!config) {
        return;
    }

    printf("Inizio deallocazione system_config...\n");

    // Libera i tipi di soccorritori
    if (config->rescuer_types_array) {
        for (int i = 0; i < config->num_rescuer_types; ++i) {
            // Libera il nome allocato dinamicamente per ciascun tipo di soccorritore
            free(config->rescuer_types_array[i].rescuer_type_name);
            config->rescuer_types_array[i].rescuer_type_name = NULL; // Buona pratica
        }
        // Libera l'array stesso dei tipi di soccorritori
        free(config->rescuer_types_array);
        config->rescuer_types_array = NULL;
        config->num_rescuer_types = 0;
        printf("  Tipi di soccorritori deallocati.\n");
    }

    // Libera i tipi di emergenze
    if (config->emergency_types_array) {
        for (int i = 0; i < config->num_emergency_types; ++i) {
            // Libera la descrizione allocata dinamicamente per ciascun tipo di emergenza
            free(config->emergency_types_array[i].emergency_desc);
            config->emergency_types_array[i].emergency_desc = NULL;

            // Libera l'array di rescuer_request_t allocato per ciascun tipo di emergenza
            // Nota: rescuer_request_t.type è un puntatore a un rescuer_type_t che è già
            // stato (o sarà) gestito dalla deallocazione di rescuer_types_array, quindi non va liberato qui.
            free(config->emergency_types_array[i].rescuers);
            config->emergency_types_array[i].rescuers = NULL;
            config->emergency_types_array[i].rescuers_req_number = 0;
        }
        // Libera l'array stesso dei tipi di emergenze
        free(config->emergency_types_array);
        config->emergency_types_array = NULL;
        config->num_emergency_types = 0;
        printf("  Tipi di emergenze deallocati.\n");
    }

    // env_config è per valore dentro system_config, quindi non serve free separato per &config->env_config.
    // queue_name è un array di char, non un puntatore allocato, quindi non va liberato.

    config->total_digital_twins_to_create = 0;

    printf("Deallocazione system_config completata.\n");
    // La struttura system_config stessa (se allocata dinamicamente)
    // dovrebbe essere liberata dal chiamante di free_system_config.
    // Se system_config_t è una variabile locale (stack), non c'è nulla da fare per essa.
}