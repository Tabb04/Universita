#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include "logger.h" // Includi il logger
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static void trim_whitespace_pet(char *str) {
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

static rescuer_type_t* find_rescuer_type_in_array_pet(const char* name, const system_config_t* config) {
    for (int i = 0; i < config->num_rescuer_types; ++i) {
        if (strcmp(config->rescuer_types_array[i].rescuer_type_name, name) == 0) {
            return &config->rescuer_types_array[i];
        }
    }
    return NULL;
}

bool parse_emergency_types(const char *filename, system_config_t *config) {
    char log_msg_buffer[MAX_LINE_LENGTH + 250];

    if (!filename || !config) {
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false;
    }
    if (config->num_rescuer_types > 0 && !config->rescuer_types_array) {
        sprintf(log_msg_buffer, "Configurazione soccorritori mancante o corrotta (num_rescuer_types=%d ma array nullo).", config->num_rescuer_types);
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false;
    }


    sprintf(log_msg_buffer, "Tentativo apertura file: %s", filename);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    FILE *file = fopen(filename, "r");
    if (!file) {
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        return false;
    }
    log_message(LOG_EVENT_FILE_PARSING, filename, "File aperto con successo.");

    char line[MAX_LINE_LENGTH];
    int line_num_pass1 = 0;
    int type_count = 0;

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio primo passaggio: conteggio tipi emergenze.");
    while (fgets(line, sizeof(line), file)) {
        line_num_pass1++;
        char count_line_buffer[MAX_LINE_LENGTH];
        strncpy(count_line_buffer, line, MAX_LINE_LENGTH-1);
        count_line_buffer[MAX_LINE_LENGTH-1] = '\0';
        count_line_buffer[strcspn(count_line_buffer, "\n")] = 0;
        trim_whitespace_pet(count_line_buffer);

        if (*count_line_buffer == '\0' || count_line_buffer[0] == '#') {
            continue;
        }
        // Controllo formato base
        char temp_name[EMERGENCY_NAME_LENGTH];
        short temp_prio;
        if (sscanf(count_line_buffer, " [%[^]]] [%hd]", temp_name, &temp_prio) >= 2) { // >= 2 perché la parte rescuers può non essere valida per sscanf
            type_count++;
        }
    }
    sprintf(log_msg_buffer, "Primo passaggio completato: Trovati %d potenziali tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    if (type_count == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Nessun tipo di emergenza trovato. Parsing completato (file vuoto o senza voci valide).");
        config->num_emergency_types = 0;
        config->emergency_types_array = NULL;
        fclose(file);
        return true;
    }

    config->emergency_types_array = (emergency_type_t *)malloc(type_count * sizeof(emergency_type_t));
    if (!config->emergency_types_array) {
        sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d emergency_type_t: %s", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        fclose(file);
        return false;
    }
    sprintf(log_msg_buffer, "Allocato array per %d tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    config->num_emergency_types = 0;

    rewind(file);
    int line_num_pass2 = 0;
    int current_type_idx = 0;

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio secondo passaggio: estrazione dati emergenze.");
    while (fgets(line, sizeof(line), file) && current_type_idx < type_count) {
        line_num_pass2++;
        char original_line_for_log[MAX_LINE_LENGTH];
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH-1);
        original_line_for_log[MAX_LINE_LENGTH-1] = '\0';
        if (strchr(original_line_for_log, '\n')) *strchr(original_line_for_log, '\n') = '\0';
        
        char temp_line_for_parsing[MAX_LINE_LENGTH];
        strcpy(temp_line_for_parsing, line); // Usiamo una copia per strtok e modifiche
        char* current_line_ptr = temp_line_for_parsing;
        current_line_ptr[strcspn(current_line_ptr, "\n")] = 0;
        trim_whitespace_pet(current_line_ptr);

        if (*current_line_ptr == '\0' || current_line_ptr[0] == '#') {
            continue;
        }

        char name_buffer[EMERGENCY_NAME_LENGTH];
        short priority_val;
        char rescuers_full_str[MAX_LINE_LENGTH] = "";

        char *parser_ptr = current_line_ptr;
        // Estrai nome
        if (*parser_ptr != '[') { goto format_error_pet; } parser_ptr++;
        char *name_end = strchr(parser_ptr, ']');
        if (!name_end || (name_end - parser_ptr) == 0 || (name_end - parser_ptr) >= EMERGENCY_NAME_LENGTH) { goto format_error_pet; }
        strncpy(name_buffer, parser_ptr, name_end - parser_ptr);
        name_buffer[name_end - parser_ptr] = '\0';
        parser_ptr = name_end + 1;

        // Estrai priorità
        while(isspace((unsigned char)*parser_ptr)) parser_ptr++;
        if (*parser_ptr != '[') { goto format_error_pet; } parser_ptr++;
        char *prio_end = strchr(parser_ptr, ']');
        if (!prio_end) { goto format_error_pet; }
        char prio_str_buf[10];
        if((prio_end - parser_ptr) == 0 || (prio_end - parser_ptr) >= sizeof(prio_str_buf)) { goto format_error_pet; }
        strncpy(prio_str_buf, parser_ptr, prio_end - parser_ptr);
        prio_str_buf[prio_end - parser_ptr] = '\0';
        if(sscanf(prio_str_buf, "%hd", &priority_val) != 1) { goto format_error_pet; }
        parser_ptr = prio_end + 1;
        
        // Estrai stringa rescuers
        while(isspace((unsigned char)*parser_ptr)) parser_ptr++;
        if(*parser_ptr != '\0') {
            strncpy(rescuers_full_str, parser_ptr, sizeof(rescuers_full_str)-1);
            rescuers_full_str[sizeof(rescuers_full_str)-1] = '\0';
        }

        if (priority_val < PRIORITY_LOW || priority_val > PRIORITY_HIGH) {
            sprintf(log_msg_buffer, "Riga %d: Priorità emergenza (%hd) non valida. Riga: '%s'. Riga ignorata.", line_num_pass2, priority_val, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue;
        }
        if (strlen(rescuers_full_str) == 0) {
            sprintf(log_msg_buffer, "Riga %d: Lista soccorritori mancante per emergenza '%s'. Riga: '%s'. Riga ignorata.", line_num_pass2, name_buffer, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue;
        }

        // --- Parsing della stringa dei soccorritori ---
        int req_array_capacity = 5;
        rescuer_request_t *requests_arr = malloc(req_array_capacity * sizeof(rescuer_request_t));
        if (!requests_arr) {
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per richieste soccorritori (emergenza '%s'): %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            goto error_cleanup_pet_fatal; // Errore fatale
        }
        int actual_req_count = 0;
        char *current_rescuer_token;
        char *rest_of_rescuers_str = rescuers_full_str; // Lavora sulla copia
        char *saveptr_token;

        while ((current_rescuer_token = strtok_r(rest_of_rescuers_str, ";", &saveptr_token)) != NULL) {
            rest_of_rescuers_str = NULL;
            trim_whitespace_pet(current_rescuer_token);
            if(strlen(current_rescuer_token) == 0) continue;

            char rt_name_buf[RESCUER_NAME_MAX_LEN];
            int num_needed_val, time_needed_val;

            if (sscanf(current_rescuer_token, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Formato richiesta soccorritore non valido ('%s'). Richiesta ignorata.", line_num_pass2, name_buffer, current_rescuer_token);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora questa singola richiesta malformata
            }
            trim_whitespace_pet(rt_name_buf);

            if (num_needed_val <= 0 || time_needed_val <= 0 || strlen(rt_name_buf) == 0) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Valori richiesta soccorritore non validi (num=%d, time=%d, nome='%s'). Richiesta ignorata.", line_num_pass2, name_buffer, num_needed_val, time_needed_val, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue;
            }

            rescuer_type_t *found_rt = find_rescuer_type_in_array_pet(rt_name_buf, config);
            if (!found_rt) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Tipo soccorritore '%s' non trovato. Richiesta ignorata.", line_num_pass2, name_buffer, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue;
            }

            if (actual_req_count >= req_array_capacity) {
                req_array_capacity *= 2;
                rescuer_request_t *temp_req_arr = realloc(requests_arr, req_array_capacity * sizeof(rescuer_request_t));
                if (!temp_req_arr) {
                    sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Fallimento riallocazione richieste soccorritori: %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
                    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                    free(requests_arr); // Libera l'array parzialmente popolato
                    goto error_cleanup_pet_fatal;
                }
                requests_arr = temp_req_arr;
            }
            requests_arr[actual_req_count].type = found_rt;
            requests_arr[actual_req_count].required_count = num_needed_val;
            requests_arr[actual_req_count].time_to_manage = time_needed_val;
            actual_req_count++;
        }

        if (actual_req_count == 0) { // Se nessuna richiesta valida è stata parsata dalla stringa rescuers
            sprintf(log_msg_buffer, "Riga %d: Nessuna richiesta soccorritore valida trovata per emergenza '%s' da stringa '%s'. Riga ignorata.", line_num_pass2, name_buffer, rescuers_full_str);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr); // requests_arr era stato allocato
            continue; // Ignora questa emergenza
        }
        
        emergency_type_t *current_et_ptr = &config->emergency_types_array[current_type_idx];
        current_et_ptr->emergency_desc = strdup(name_buffer);
        if (!current_et_ptr->emergency_desc) {
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per nome emergenza '%s': %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr); // Libera l'array di richieste per questa emergenza
            goto error_cleanup_pet_fatal;
        }
        current_et_ptr->priority = priority_val;
        current_et_ptr->rescuers = requests_arr;
        current_et_ptr->rescuers_req_number = actual_req_count;

        sprintf(log_msg_buffer, "Riga %d: Estratta emergenza: Nome='%s', Priorità=%hd, N.Richieste Socc.=%d",
                line_num_pass2, current_et_ptr->emergency_desc, current_et_ptr->priority, current_et_ptr->rescuers_req_number);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        for(int k=0; k < actual_req_count; ++k) {
            sprintf(log_msg_buffer, "    Richiesta %d: Tipo='%s', Num=%d, Tempo=%d", k+1, requests_arr[k].type->rescuer_type_name, requests_arr[k].required_count, requests_arr[k].time_to_manage);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        }
        
        current_type_idx++;
        continue;

    format_error_pet:
        sprintf(log_msg_buffer, "Riga %d: Errore formato: '%s'. Riga ignorata.", line_num_pass2, original_line_for_log);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        // Non incrementare current_type_idx, continua con la prossima riga
        continue; 
    }
    config->num_emergency_types = current_type_idx; // Numero effettivo di tipi validi letti

    fclose(file);

    if (config->num_emergency_types == 0 && type_count > 0) {
        sprintf(log_msg_buffer, "Nessun tipo di emergenza valido estratto nel secondo passaggio, nonostante %d potenziali. Parsing fallito.", type_count);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        free(config->emergency_types_array);
        config->emergency_types_array = NULL;
        return false;
    }

    sprintf(log_msg_buffer, "Completamento parsing: Letti %d tipi di emergenze validi.", config->num_emergency_types);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    return true;

error_cleanup_pet_fatal: // Usato per errori di allocazione fatali che interrompono il parsing
    for (int i = 0; i < current_type_idx; ++i) { // Libera tipi di emergenze già completamente allocati
        if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
        if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
    }
    if (config->emergency_types_array) free(config->emergency_types_array);
    config->emergency_types_array = NULL;
    config->num_emergency_types = 0;
    if(file) fclose(file);
    return false;
}