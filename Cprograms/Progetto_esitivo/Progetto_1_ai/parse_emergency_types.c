#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h> // Per isspace

// Funzione helper interna per trovare un tipo di soccorritore per nome
static rescuer_type_t* find_rescuer_type_by_name(const char* name, const rescuer_type_t* list) {
    const rescuer_type_t* current = list;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return (rescuer_type_t*)current; // Cast sicuro perché non modifichiamo la lista originale
        }
        current = current->next;
    }
    return NULL; // Non trovato
}

// Funzione helper per liberare la memoria allocata per la lista e i suoi contenuti
void free_emergency_type_list(emergency_type_t *head) {
    emergency_type_t *current = head;
    emergency_type_t *next;
    while (current != NULL) {
        next = current->next;
        // Libera l'array di rescuer_request_t
        free(current->rescuers);
        // Il nome è un array fisso, non serve free
        // Libera la struttura emergency_type_t stessa
        free(current);
        current = next;
    }
}


bool parse_emergency_types(const char *filename, const rescuer_type_t *available_rescuers, emergency_type_t **emergency_type_list) {
    if (!filename || !available_rescuers || !emergency_type_list) {
        fprintf(stderr, "Errore: Argomenti nulli passati a parse_emergency_types.\n");
        return false;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file emergency_types");
        fprintf(stderr, "Impossibile aprire il file di configurazione tipi emergenze: %s\n", filename);
        return false;
    }

    *emergency_type_list = NULL; // Inizializza la lista come vuota
    emergency_type_t *current_et = NULL;
    emergency_type_t *tail = NULL; // Puntatore all'ultimo nodo per aggiunte O(1)

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0; // Rimuove newline

        // Ignora righe vuote o commenti
        char *trimmed_line = line;
        while(isspace((unsigned char)*trimmed_line)) trimmed_line++;
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }

        // Expected format: [<name>] [<priority>] <rescuers>
        // <rescuers> ::= <rescuertype>:<number>,<time_in_secs>;...

        char name[EMERGENCY_NAME_LENGTH];
        short priority;
        char rescuers_str[MAX_LINE_LENGTH]; // Stringa che contiene le richieste soccorritori

        // Trova l'inizio della stringa dei soccorritori
        char *first_bracket = strchr(trimmed_line, '[');
        if (!first_bracket) { /* ... errore formato ... */ continue; }
        char *first_closing_bracket = strchr(first_bracket + 1, ']');
        if (!first_closing_bracket) { /* ... errore formato ... */ continue; }

        char *second_bracket = strchr(first_closing_bracket + 1, '[');
        if (!second_bracket) { /* ... errore formato ... */ continue; }
        char *second_closing_bracket = strchr(second_bracket + 1, ']');
        if (!second_closing_bracket) { /* ... errore formato ... */ continue; }

        // Estrai nome
        int name_len = first_closing_bracket - (first_bracket + 1);
        if (name_len <= 0 || name_len >= EMERGENCY_NAME_LENGTH) {
            fprintf(stderr, "Errore parsing emergency_types: Nome emergenza non valido o troppo lungo alla riga %d\n", line_num);
            goto error_cleanup;
        }
        strncpy(name, first_bracket + 1, name_len);
        name[name_len] = '\0';

        // Estrai priorità
        if (sscanf(second_bracket + 1, "%hd", &priority) != 1) {
             fprintf(stderr, "Errore parsing emergency_types: Priorità non valida alla riga %d\n", line_num);
             goto error_cleanup;
        }
        // Validazione priorità
        if (priority < PRIORITY_LOW || priority > PRIORITY_HIGH) {
             fprintf(stderr, "Errore parsing emergency_types: Valore priorità (%hd) non consentito alla riga %d (ammessi %d, %d, %d)\n",
                     priority, line_num, PRIORITY_LOW, PRIORITY_MEDIUM, PRIORITY_HIGH);
             goto error_cleanup;
        }

        // Estrai la stringa dei soccorritori (ciò che viene dopo la seconda parentesi chiusa)
        char* rescuers_part = second_closing_bracket + 1;
        while(isspace((unsigned char)*rescuers_part)) rescuers_part++; // Salta spazi
        strncpy(rescuers_str, rescuers_part, sizeof(rescuers_str) - 1);
        rescuers_str[sizeof(rescuers_str) - 1] = '\0';

        if (strlen(rescuers_str) == 0) {
            fprintf(stderr, "Errore parsing emergency_types: Lista soccorritori mancante per '%s' alla riga %d\n", name, line_num);
            goto error_cleanup;
        }

        // --- Parsing della stringa dei soccorritori ---
        int req_capacity = 5; // Capacità iniziale per l'array di richieste
        rescuer_request_t *requests = malloc(req_capacity * sizeof(rescuer_request_t));
        if (!requests) {
            perror("Errore allocazione memoria per rescuer_request_t array");
            goto error_cleanup;
        }
        int req_count = 0;

        char *req_token;
        char *rest = rescuers_str;
        char *saveptr_req; // Per strtok_r

        // Splitta per ';'
        while ((req_token = strtok_r(rest, ";", &saveptr_req)) != NULL) {
            rest = NULL; // Per le chiamate successive a strtok_r

            char rescuer_type_name[RESCUER_NAME_MAX_LEN];
            int num_needed, time_needed;

            // Parsing interno: <type>:<num>,<time>
            // Usiamo sscanf sulla copia locale req_token
            if (sscanf(req_token, " %[^:]:%d,%d", rescuer_type_name, &num_needed, &time_needed) != 3) {
                 fprintf(stderr, "Errore parsing emergency_types: Formato richiesta soccorritore non valido ('%s') per '%s' alla riga %d\n",
                         req_token, name, line_num);
                 free(requests); // Libera l'array delle richieste parzialmente popolato
                 goto error_cleanup;
            }

             // Validazione valori numerici
            if (num_needed <= 0 || time_needed <= 0) {
                 fprintf(stderr, "Errore parsing emergency_types: Valori numerici richiesta soccorritore non validi (num=%d, time=%d) per '%s' alla riga %d\n",
                         num_needed, time_needed, name, line_num);
                 free(requests);
                 goto error_cleanup;
            }

            // Cerca il tipo di soccorritore corrispondente nella lista fornita
            rescuer_type_t *found_type = find_rescuer_type_by_name(rescuer_type_name, available_rescuers);
            if (!found_type) {
                fprintf(stderr, "Errore parsing emergency_types: Tipo soccorritore '%s' richiesto per '%s' (riga %d) non definito in rescuers.conf\n",
                        rescuer_type_name, name, line_num);
                free(requests);
                goto error_cleanup;
            }

            // Controlla se l'array di richieste è pieno, rialloca se necessario
            if (req_count >= req_capacity) {
                req_capacity *= 2;
                rescuer_request_t *temp = realloc(requests, req_capacity * sizeof(rescuer_request_t));
                if (!temp) {
                    perror("Errore riallocazione memoria per rescuer_request_t array");
                    free(requests);
                    goto error_cleanup;
                }
                requests = temp;
            }

            // Popola la prossima richiesta nell'array
            requests[req_count].type = found_type; // Salva il puntatore al tipo trovato
            requests[req_count].required_count = num_needed;
            requests[req_count].time_to_manage = time_needed;
            req_count++;
        } // Fine loop strtok_r per ';'

        if (req_count == 0) {
             fprintf(stderr, "Errore parsing emergency_types: Nessuna richiesta soccorritore valida trovata per '%s' alla riga %d\n", name, line_num);
             free(requests); // Libera l'array vuoto
             goto error_cleanup;
        }

        // --- Fine Parsing Stringa Soccorritori ---

        // Crea un nuovo nodo per il tipo di emergenza
        current_et = (emergency_type_t *)malloc(sizeof(emergency_type_t));
        if (!current_et) {
            perror("Errore allocazione memoria per emergency_type_t");
            free(requests); // Libera l'array delle richieste appena creato
            goto error_cleanup;
        }

        // Copia i dati nel nuovo nodo
        strncpy(current_et->emergency_desc, name, EMERGENCY_NAME_LENGTH - 1);
        current_et->emergency_desc[EMERGENCY_NAME_LENGTH - 1] = '\0';
        current_et->priority = priority;
        current_et->rescuers = requests; // Assegna l'array di richieste allocato
        current_et->rescuers_req_number = req_count;
        current_et->next = NULL;

        // Aggiunge il nodo alla lista linkata
        if (*emergency_type_list == NULL) {
            *emergency_type_list = current_et; // Primo nodo
            tail = current_et;
        } else {
            tail->next = current_et; // Aggiunge in coda
            tail = current_et;       // Aggiorna la coda
        }

        printf("  Letto tipo emergenza: %s (Priorità: %d, Richieste: %d)\n",
               current_et->emergency_desc, current_et->priority, current_et->rescuers_req_number);
        // Optional: Print details of requests
        // for(int i=0; i<current_et->rescuers_req_number; ++i) {
        //     printf("    - %s: %d unità per %d sec\n", current_et->rescuers[i].type->name, current_et->rescuers[i].required_count, current_et->rescuers[i].time_to_manage);
        // }

    } // Fine loop fgets

    fclose(file);

    if (*emergency_type_list == NULL) {
        fprintf(stderr, "Attenzione: Nessun tipo di emergenza definito nel file %s.\n", filename);
        // Potrebbe essere valido o meno, non è un errore fatale qui.
    }

    printf("Parsing tipi emergenza completato.\n");
    return true;

error_cleanup: // Etichetta per gestire gli errori liberando memoria
    fprintf(stderr, "Errore durante il parsing del file tipi emergenze %s.\n", filename);
    free_emergency_type_list(*emergency_type_list); // Libera qualsiasi cosa sia stata allocata finora
    *emergency_type_list = NULL;
    if(file) fclose(file); // Assicura che il file venga chiuso
    return false;
}