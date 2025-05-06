#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

// Funzione trim_whitespace (può essere duplicata o messa in un file utils.c comune)
static void trim_whitespace_et(char *str) { // rinominata
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

// Funzione helper interna per trovare un tipo di soccorritore per nome nell'array
static rescuer_type_t* find_rescuer_type_in_array(const char* name, const system_config_t* config) {
    for (int i = 0; i < config->num_rescuer_types; ++i) {
        if (strcmp(config->rescuer_types_array[i].rescuer_type_name, name) == 0) {
            return &config->rescuer_types_array[i];
        }
    }
    return NULL; // Non trovato
}

bool parse_emergency_types(const char *filename, system_config_t *config) {
    if (!filename || !config || (!config->rescuer_types_array && config->num_rescuer_types > 0) ) {
        fprintf(stderr, "Errore: Argomenti nulli o configurazione soccorritori mancante passati a parse_emergency_types.\n");
        return false;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file emergency_types");
        fprintf(stderr, "Impossibile aprire il file di configurazione tipi emergenze: %s\n", filename);
        return false;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    int type_count = 0;

    // Primo Passaggio: Contare il numero di tipi di emergenze validi
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line = line;
        trim_whitespace_et(trimmed_line);
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }
        // Controllo base del formato
        char name_buf[EMERGENCY_NAME_LENGTH];
        short prio_buf;
        if (sscanf(trimmed_line, " [%[^]]] [%hd]", name_buf, &prio_buf) < 2) { // <2 perché rescuers_str può essere vuoto
            // Non contare se la parte iniziale non matcha
        } else {
            type_count++;
        }
    }

    if (type_count == 0) {
        fprintf(stdout, "Attenzione: Nessun tipo di emergenza trovato nel file %s.\n", filename);
        config->num_emergency_types = 0;
        config->emergency_types_array = NULL;
        fclose(file);
        return true; // Non un errore fatale
    }

    config->emergency_types_array = (emergency_type_t *)malloc(type_count * sizeof(emergency_type_t));
    if (!config->emergency_types_array) {
        perror("Errore allocazione memoria per emergency_types_array");
        fclose(file);
        return false;
    }
    config->num_emergency_types = 0; // Sarà incrementato

    rewind(file);
    line_num = 0;
    int current_type_idx = 0;

    // Secondo Passaggio: Leggere e popolare i dati
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line_orig = line; // Conserva l'originale per messaggi di errore
        
        char temp_line_for_parsing[MAX_LINE_LENGTH];
        strcpy(temp_line_for_parsing, trimmed_line_orig);
        char* trimmed_line = temp_line_for_parsing;

        trim_whitespace_et(trimmed_line);
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }

        char name_buffer[EMERGENCY_NAME_LENGTH];
        short priority_val;
        char rescuers_full_str[MAX_LINE_LENGTH] = ""; // Inizializza a stringa vuota

        // Parsing più robusto per nome, priorità, e stringa rescuers
        char *ptr = trimmed_line;
        if (*ptr != '[') goto format_error; ptr++; // Salta '['
        char *name_end = strchr(ptr, ']');
        if (!name_end) goto format_error;
        if ((name_end - ptr) >= EMERGENCY_NAME_LENGTH || (name_end - ptr) == 0) goto format_error; // Check lunghezza nome
        strncpy(name_buffer, ptr, name_end - ptr);
        name_buffer[name_end - ptr] = '\0';
        ptr = name_end + 1;

        while(isspace((unsigned char)*ptr)) ptr++; // Salta spazi
        if (*ptr != '[') goto format_error; ptr++; // Salta '['
        char *prio_end = strchr(ptr, ']');
        if (!prio_end) goto format_error;
        char prio_str_buf[10]; // Buffer per la stringa della priorità
        if((prio_end - ptr) >= sizeof(prio_str_buf) || (prio_end - ptr) == 0) goto format_error;
        strncpy(prio_str_buf, ptr, prio_end - ptr);
        prio_str_buf[prio_end - ptr] = '\0';
        if(sscanf(prio_str_buf, "%hd", &priority_val) != 1) goto format_error;
        ptr = prio_end + 1;

        while(isspace((unsigned char)*ptr)) ptr++; // Salta spazi
        if(*ptr != '\0') { // Se c'è la parte dei soccorritori
            strncpy(rescuers_full_str, ptr, sizeof(rescuers_full_str)-1);
            rescuers_full_str[sizeof(rescuers_full_str)-1] = '\0';
        }


        if (priority_val < PRIORITY_LOW || priority_val > PRIORITY_HIGH) {
            fprintf(stderr, "Errore parsing emergency_types: Valore priorità (%hd) non consentito alla riga %d\n", priority_val, line_num);
            goto error_cleanup_et;
        }
        if (strlen(name_buffer) == 0) {
             fprintf(stderr, "Errore parsing emergency_types: Nome emergenza vuoto alla riga %d\n", line_num);
             goto error_cleanup_et;
        }
        if (strlen(rescuers_full_str) == 0) { // Ogni emergenza deve richiedere almeno un soccorritore? Da specifiche sembrerebbe di sì.
            fprintf(stderr, "Errore parsing emergency_types: Lista soccorritori mancante per '%s' alla riga %d\n", name_buffer, line_num);
            goto error_cleanup_et;
        }


        // --- Parsing della stringa dei soccorritori ---
        int req_array_capacity = 5;
        rescuer_request_t *requests_arr = malloc(req_array_capacity * sizeof(rescuer_request_t));
        if (!requests_arr) {
            perror("Errore allocazione iniziale per requests_arr");
            goto error_cleanup_et;
        }
        int actual_req_count = 0;
        char *current_rescuer_token;
        char *rest_of_rescuers_str = rescuers_full_str;
        char *saveptr_token;

        while ((current_rescuer_token = strtok_r(rest_of_rescuers_str, ";", &saveptr_token)) != NULL) {
            rest_of_rescuers_str = NULL; // Per chiamate successive a strtok_r
            trim_whitespace_et(current_rescuer_token);
            if(strlen(current_rescuer_token) == 0) continue; // Ignora token vuoti (es. due ';')

            char rt_name_buf[RESCUER_NAME_MAX_LEN];
            int num_needed_val, time_needed_val;

            if (sscanf(current_rescuer_token, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3) {
                fprintf(stderr, "Errore parsing emergency_types: Formato richiesta soccorritore ('%s') non valido per '%s' riga %d\n",
                        current_rescuer_token, name_buffer, line_num);
                free(requests_arr);
                goto error_cleanup_et;
            }
             trim_whitespace_et(rt_name_buf); // Pulisce il nome del tipo soccorritore

            if (num_needed_val <= 0 || time_needed_val <= 0 || strlen(rt_name_buf) == 0) {
                fprintf(stderr, "Errore parsing emergency_types: Valori richiesta soccorritore non validi per '%s' riga %d\n", name_buffer, line_num);
                free(requests_arr);
                goto error_cleanup_et;
            }

            rescuer_type_t *found_rt = find_rescuer_type_in_array(rt_name_buf, config);
            if (!found_rt) {
                fprintf(stderr, "Errore parsing emergency_types: Tipo soccorritore '%s' (per '%s' riga %d) non definito in rescuers.conf\n",
                        rt_name_buf, name_buffer, line_num);
                free(requests_arr);
                goto error_cleanup_et;
            }

            if (actual_req_count >= req_array_capacity) {
                req_array_capacity *= 2;
                rescuer_request_t *temp_req_arr = realloc(requests_arr, req_array_capacity * sizeof(rescuer_request_t));
                if (!temp_req_arr) {
                    perror("Errore riallocazione requests_arr");
                    free(requests_arr);
                    goto error_cleanup_et;
                }
                requests_arr = temp_req_arr;
            }
            requests_arr[actual_req_count].type = found_rt;
            requests_arr[actual_req_count].required_count = num_needed_val;
            requests_arr[actual_req_count].time_to_manage = time_needed_val;
            actual_req_count++;
        } // Fine loop strtok_r

        if (actual_req_count == 0 && strlen(rescuers_full_str) > 0) { // Se c'era una stringa ma non sono stati parsati token validi
             fprintf(stderr, "Errore parsing emergency_types: Nessuna richiesta soccorritore valida parsata da '%s' per '%s' riga %d\n", rescuers_full_str, name_buffer, line_num);
             free(requests_arr);
             goto error_cleanup_et;
        }


        // --- Fine Parsing Stringa Soccorritori ---
        emergency_type_t *current_et_ptr = &config->emergency_types_array[current_type_idx];
        current_et_ptr->emergency_desc = strdup(name_buffer);
        if (!current_et_ptr->emergency_desc) {
            perror("Errore allocazione memoria per emergency_desc");
            free(requests_arr);
            goto error_cleanup_et;
        }
        current_et_ptr->priority = priority_val;
        current_et_ptr->rescuers = requests_arr; // Array allocato sopra
        current_et_ptr->rescuers_req_number = actual_req_count;

        current_type_idx++;
        config->num_emergency_types = current_type_idx;

        printf("  Letto tipo emergenza: %s (Priorità: %d, N.Richieste: %d)\n",
               current_et_ptr->emergency_desc, current_et_ptr->priority, current_et_ptr->rescuers_req_number);
        continue; // Vai alla prossima riga

    format_error:
        fprintf(stderr, "Errore parsing emergency_types: Formato linea non valido alla riga %d: '%s'\n", line_num, trimmed_line_orig);
    error_cleanup_et: // Etichetta per gestire gli errori liberando memoria
        // Libera tutto ciò che è stato allocato finora per emergency_types_array
        for (int i = 0; i < current_type_idx; ++i) {
            free(config->emergency_types_array[i].emergency_desc);
            free(config->emergency_types_array[i].rescuers); // Libera l'array interno
        }
        free(config->emergency_types_array);
        config->emergency_types_array = NULL;
        config->num_emergency_types = 0;
        if(file) fclose(file);
        return false;

    } // Fine while fgets

    fclose(file);

    if (config->num_emergency_types != type_count && type_count > 0) {
        fprintf(stderr, "Discordanza nel conteggio dei tipi di emergenza. Pulizia.\n");
         for (int i = 0; i < config->num_emergency_types; ++i) {
            free(config->emergency_types_array[i].emergency_desc);
            free(config->emergency_types_array[i].rescuers);
        }
        free(config->emergency_types_array);
        config->emergency_types_array = NULL;
        config->num_emergency_types = 0;
        return false;
    }


    printf("Parsing tipi emergenza completato. Tipi letti: %d\n", config->num_emergency_types);
    return true;
}