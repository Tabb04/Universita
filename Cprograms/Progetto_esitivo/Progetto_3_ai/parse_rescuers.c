#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include "logger.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

// Funzione trim_whitespace (può essere duplicata o messa in un file utils.c comune)
static void trim_whitespace_prs(char *str) {
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

bool parse_rescuers(const char *filename, system_config_t *config) {
    // Buffer per costruire i messaggi di log prima di inviarli alla funzione log_message
    char log_msg_buffer[MAX_LINE_LENGTH + 200];

    // 1. Validazione degli Argomenti di Input
    if (!filename || !config) {
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_rescuers", log_msg_buffer);
        return false; // Indica fallimento
    }

    // 2. Apertura del File di Configurazione
    sprintf(log_msg_buffer, "Tentativo apertura file: %s", filename);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer); // Log: filename è l'ID qui

    FILE *file = fopen(filename, "r"); // Apre in modalità lettura ("r")
    if (!file) {
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno));
        // strerror(errno) fornisce una descrizione testuale dell'ultimo errore di sistema
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        return false; // Fallimento
    }
    log_message(LOG_EVENT_FILE_PARSING, filename, "File aperto con successo.");

    // 3. Primo Passaggio: Conteggio dei Tipi di Soccorritori
    // Questo passaggio serve a determinare quanti tipi di soccorritori sono definiti nel file,
    // per poter allocare correttamente la memoria per l'array `config->rescuer_types_array`.
    char line[MAX_LINE_LENGTH];  // Buffer per leggere una riga alla volta
    int line_num_pass1 = 0;      // Contatore di righe per il primo passaggio
    int type_count = 0;          // Contatore dei tipi di soccorritori potenzialmente validi

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio primo passaggio: conteggio tipi soccorritori.");
    while (fgets(line, sizeof(line), file)) { // Legge una riga
        line_num_pass1++;
        
        char count_line_buffer[MAX_LINE_LENGTH]; // Buffer di lavoro per non alterare 'line'
        strncpy(count_line_buffer, line, MAX_LINE_LENGTH -1);
        count_line_buffer[MAX_LINE_LENGTH-1] = '\0'; // Assicura terminazione null
        
        count_line_buffer[strcspn(count_line_buffer, "\n")] = 0; // Rimuove il newline
        trim_whitespace_prs(count_line_buffer); // Rimuove spazi bianchi esterni

        if (*count_line_buffer == '\0' || count_line_buffer[0] == '#') {
            continue; // Ignora righe vuote o commenti
        }

        // Controllo di formato MOLTO basilare solo per il conteggio.
        // Una riga che *sembra* un entry di soccorritore viene contata.
        // La validazione completa avverrà nel secondo passaggio.
        char temp_name[RESCUER_NAME_MAX_LEN]; // Variabili temporanee per sscanf
        int temp_num, temp_speed, temp_x, temp_y;
        if (sscanf(count_line_buffer, " [%[^]]] [%d] [%d] [%d;%d]", temp_name, &temp_num, &temp_speed, &temp_x, &temp_y) == 5) {
            // sscanf tenta di matchare 5 campi. Se ci riesce, incrementa type_count.
            // %*[^]] legge una stringa fino a ']' ma non la assegna.
            // Qui usiamo %[^]] per estrarre effettivamente il nome in temp_name.
            type_count++;
        }
    }
    sprintf(log_msg_buffer, "Primo passaggio completato: Trovati %d potenziali tipi di soccorritori.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    // 4. Gestione del Caso: Nessun Tipo Trovato
    if (type_count == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Nessun tipo di soccorritore trovato. Parsing completato (file vuoto o senza voci valide).");
        config->num_rescuer_types = 0;
        config->rescuer_types_array = NULL; // Nessun array da allocare
        config->total_digital_twins_to_create = 0;
        fclose(file); // Chiude il file
        return true; // Non è un errore fatale, il file potrebbe essere legittimamente vuoto
    }

    // 5. Allocazione Memoria per l'Array dei Tipi di Soccorritori
    // Ora che sappiamo quanti tipi ci sono (type_count), allochiamo l'array.
    config->rescuer_types_array = (rescuer_type_t *)malloc(type_count * sizeof(rescuer_type_t));
    if (!config->rescuer_types_array) {
        sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d rescuer_type_t: %s", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        fclose(file);
        return false; // Errore fatale di memoria
    }
    sprintf(log_msg_buffer, "Allocato array per %d tipi di soccorritori.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    // Inizializza i contatori per il secondo passaggio
    config->num_rescuer_types = 0; // Verrà incrementato per ogni tipo *validamente* letto
    config->total_digital_twins_to_create = 0;

    // 6. Secondo Passaggio: Lettura ed Estrazione Dati
    rewind(file); // Riporta il puntatore del file all'inizio per rileggere il file
    int line_num_pass2 = 0;     // Contatore di righe per il secondo passaggio
    int current_type_idx = 0; // Indice per popolare `config->rescuer_types_array`

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio secondo passaggio: estrazione dati soccorritori.");
    // Il loop continua finché ci sono righe da leggere E non abbiamo ancora riempito
    // l'array allocato (current_type_idx < type_count).
    while (fgets(line, sizeof(line), file) && current_type_idx < type_count) {
        line_num_pass2++;
        char original_line_for_log[MAX_LINE_LENGTH]; // Conserva la riga originale per i log di errore
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH -1);
        original_line_for_log[MAX_LINE_LENGTH-1] = '\0';
        // Rimuove il newline dalla copia per il log, se presente
        if (strchr(original_line_for_log, '\n')) *strchr(original_line_for_log, '\n') = '\0';

        line[strcspn(line, "\n")] = 0;      // Rimuove newline dalla riga di lavoro
        char *trimmed_line = line;          // Puntatore alla riga di lavoro
        trim_whitespace_prs(trimmed_line); // Pulisce la riga di lavoro

        if (*trimmed_line == '\0' || trimmed_line[0] == '#') {
            continue; // Ignora righe vuote o commenti
        }

        // Variabili per conservare i dati estratti dalla riga corrente
        char name_buffer[RESCUER_NAME_MAX_LEN]; // Buffer temporaneo per il nome
        int num_instances; // Numero di unità di questo tipo
        int speed;
        int base_x, base_y;

        // Tenta di estrarre i 5 campi attesi usando sscanf
        int matched = sscanf(trimmed_line, " [%[^]]] [%d] [%d] [%d;%d]",
                             name_buffer, &num_instances, &speed, &base_x, &base_y);

        if (matched != 5) { // Se sscanf non ha estratto 5 campi, il formato è errato
            sprintf(log_msg_buffer, "Riga %d: Errore formato: '%s'. Attesi 5 campi. Riga ignorata.", line_num_pass2, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga e passa alla successiva. Non è un errore fatale per il parsing generale.
        }

        // Validazione dei valori estratti
        if (num_instances <= 0 || speed <= 0 || base_x < 0 || base_y < 0 || strlen(name_buffer) == 0 || strlen(name_buffer) >= RESCUER_NAME_MAX_LEN) {
            sprintf(log_msg_buffer, "Riga %d: Valori non validi per soccorritore (num=%d, speed=%d, base_x=%d, base_y=%d, nome_len=%zu). Riga: '%s'. Riga ignorata.",
                    line_num_pass2, num_instances, speed, base_x, base_y, strlen(name_buffer), original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga con dati non validi
        }

        // Se la riga è valida, popola la prossima struttura rescuer_type_t nell'array
        rescuer_type_t *current_rt = &config->rescuer_types_array[current_type_idx];
        
        // Il nome deve essere allocato dinamicamente perché rescuer_type_name è char*
        current_rt->rescuer_type_name = strdup(name_buffer); // strdup = malloc + strcpy
        if (!current_rt->rescuer_type_name) { // Se strdup fallisce (mancanza di memoria)
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per nome soccorritore '%s': %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            // Cleanup: libera i nomi già allocati nell'array e l'array stesso
            for (int i = 0; i < current_type_idx; ++i) {
                free(config->rescuer_types_array[i].rescuer_type_name);
            }
            free(config->rescuer_types_array);
            config->rescuer_types_array = NULL;
            config->num_rescuer_types = 0; // Resetta perché la lista è invalida
            fclose(file);
            return false; // Errore fatale
        }
        // Popola gli altri campi della struttura
        current_rt->speed = speed;
        current_rt->x = base_x;
        current_rt->y = base_y;
        
        // Aggiorna il conteggio totale dei gemelli digitali da creare.
        // num_instances non viene memorizzato in rescuer_type_t secondo le nuove specifiche.
        config->total_digital_twins_to_create += num_instances;
        
        // Log del tipo di soccorritore estratto con successo
        sprintf(log_msg_buffer, "Riga %d: Estratto soccorritore: Nome='%s', Speed=%d, Base=(%d;%d). Istanze per questo tipo: %d",
                line_num_pass2, current_rt->rescuer_type_name, current_rt->speed, current_rt->x, current_rt->y, num_instances);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        
        current_type_idx++; // Incrementa l'indice solo se la riga è stata processata con successo
    }
    // Dopo il loop, il numero effettivo di tipi validi letti è current_type_idx
    config->num_rescuer_types = current_type_idx;

    // 7. Chiusura del File
    fclose(file);

    // 8. Controllo Finale Post-Parsing
    // Se type_count (stima iniziale) > 0 ma num_rescuer_types (effettivi) è 0,
    // significa che tutte le righe potenziali erano errate.
    if (config->num_rescuer_types == 0 && type_count > 0) {
        sprintf(log_msg_buffer, "Nessun tipo di soccorritore valido estratto nel secondo passaggio, nonostante %d potenziali nel primo. Parsing fallito.", type_count);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        free(config->rescuer_types_array); // L'array era stato allocato
        config->rescuer_types_array = NULL;
        return false; // Considerato un fallimento se ci si aspettava qualcosa
    }
    
    // Log del completamento del parsing del file
    sprintf(log_msg_buffer, "Completamento parsing: Letti %d tipi di soccorritori validi. Totale gemelli digitali da creare: %d.",
           config->num_rescuer_types, config->total_digital_twins_to_create);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    return true; // Successo
}


/*
// Helper per il parsing di rescuers: prima contiamo gli elementi, poi allochiamo
bool parse_rescuers(const char *filename, system_config_t *config) {
    char log_msg_buffer[MAX_LINE_LENGTH + 200];

    if (!filename || !config) {
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_rescuers", log_msg_buffer);
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



    // Primo Passaggio: Contare il numero di tipi di soccorritori validi

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio primo passaggio: conteggio tipi soccorritori.");

    while (fgets(line, sizeof(line), file)) {
        line_num_pass1++;
        char count_line_buffer[MAX_LINE_LENGTH]; // Buffer per non modificare 'line' per sscanf
        strncpy(count_line_buffer, line, MAX_LINE_LENGTH -1);
        count_line_buffer[MAX_LINE_LENGTH-1] = '\0';
        
        count_line_buffer[strcspn(count_line_buffer, "\n")] = 0; // Rimuovi newline per il trim
        trim_whitespace_prs(count_line_buffer);
        
        if (*count_line_buffer == '\0' || count_line_buffer[0] == '#') {
            continue; // Ignora righe vuote o commenti
        }

        // Controllo formato base per il conteggio
        // Non logghiamo errori di formato qui, lo faremo nel secondo passaggio
        char temp_name[RESCUER_NAME_MAX_LEN];
        int temp_num, temp_speed, temp_x, temp_y;
        if (sscanf(count_line_buffer, " [%[^]]] [%d] [%d] [%d;%d]", temp_name, &temp_num, &temp_speed, &temp_x, &temp_y) == 5) {
            type_count++;
        }
    }

    sprintf(log_msg_buffer, "Primo passaggio completato: Trovati %d potenziali tipi di soccorritori.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    if (type_count == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Nessun tipo di soccorritore trovato. Parsing completato (file vuoto o senza voci valide).");
        config->num_rescuer_types = 0;
        config->rescuer_types_array = NULL;
        config->total_digital_twins_to_create = 0;
        fclose(file);
        return true;
    }

    config->rescuer_types_array = (rescuer_type_t *)malloc(type_count * sizeof(rescuer_type_t));
    if (!config->rescuer_types_array) {
        sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d rescuer_type_t: %s", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        fclose(file);
        return false;
    }

    sprintf(log_msg_buffer, "Allocato array per %d tipi di soccorritori.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    config->num_rescuer_types = 0;
    config->total_digital_twins_to_create = 0;

    // Riporta il file pointer all'inizio
    rewind(file);
    int line_num_pass2 = 0;
    int current_type_idx = 0;

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio secondo passaggio: estrazione dati soccorritori.");

    // Secondo Passaggio: Leggere e popolare i dati
    while (fgets(line, sizeof(line), file) && current_type_idx < type_count) {
        
        line_num_pass2++;
        char original_line_for_log[MAX_LINE_LENGTH];
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH -1);
        original_line_for_log[MAX_LINE_LENGTH-1] = '\0';
        if (strchr(original_line_for_log, '\n')) *strchr(original_line_for_log, '\n') = '\0';
        
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line = line;
        trim_whitespace_prs(trimmed_line);

        if (*trimmed_line == '\0' || trimmed_line[0] == '#') {
            continue;
        }

        char name_buffer[RESCUER_NAME_MAX_LEN];
        int num_instances, speed, base_x, base_y;


        int matched = sscanf(trimmed_line, " [%[^]]] [%d] [%d] [%d;%d]",
                             name_buffer, &num_instances, &speed, &base_x, &base_y);

        if (matched != 5) {
            sprintf(log_msg_buffer, "Riga %d: Errore formato: '%s'. Attesi 5 campi. Riga ignorata.", line_num_pass2, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            // Non fatale per ora, continua con la prossima riga, ma non incrementare current_type_idx
            // Se si volesse essere più severi, si potrebbe liberare e fallire.
            // Per ora, il conteggio iniziale era solo una stima massima.
            continue; // Ignora questa riga malformata e non la conta
        }

        if (num_instances <= 0 || speed <= 0 || base_x < 0 || base_y < 0 || strlen(name_buffer) == 0 || strlen(name_buffer) >= RESCUER_NAME_MAX_LEN) {
            sprintf(log_msg_buffer, "Riga %d: Valori non validi per soccorritore (num=%d, speed=%d, base_x=%d, base_y=%d, nome_len=%zu). Riga: '%s'. Riga ignorata.",
                    line_num_pass2, num_instances, speed, base_x, base_y, strlen(name_buffer), original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga con dati non validi
        }

        rescuer_type_t *current_rt = &config->rescuer_types_array[current_type_idx];
        
        current_rt->rescuer_type_name = strdup(name_buffer); // Alloca e copia nome
        if (!current_rt->rescuer_type_name) {
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per nome soccorritore '%s': %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            for (int i = 0; i < current_type_idx; ++i) { // Libera nomi già allocati
                free(config->rescuer_types_array[i].rescuer_type_name);
            }
            free(config->rescuer_types_array);
            config->rescuer_types_array = NULL;
            config->num_rescuer_types = 0;
            fclose(file);
            return false;
        }

        current_rt->speed = speed;
        current_rt->x = base_x;
        current_rt->y = base_y;
        // Il numero di istanze (num_instances) non è nella struct rescuer_type_t
        // Lo usiamo per calcolare il totale
        config->total_digital_twins_to_create += num_instances;
        
        current_type_idx++;
        config->num_rescuer_types = current_type_idx; // Aggiorna il conteggio effettivo

        sprintf(log_msg_buffer, "Riga %d: Estratto soccorritore: Nome='%s', Speed=%d, Base=(%d;%d). Istanze per questo tipo: %d",
                line_num_pass2, current_rt->rescuer_type_name, current_rt->speed, current_rt->x, current_rt->y, num_instances);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

        current_type_idx++; // Solo se la riga è valida e processata

    } // fine while fgets

    config->num_rescuer_types = current_type_idx; // Numero effettivo di tipi validi letti

    fclose(file);

    if (config->num_rescuer_types == 0 && type_count > 0) {
        // Se il conteggio iniziale era > 0 ma non abbiamo letto nessun tipo valido nel secondo pass
        sprintf(log_msg_buffer, "Nessun tipo di soccorritore valido estratto nel secondo passaggio, nonostante %d potenziali nel primo. Parsing fallito.", type_count);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        free(config->rescuer_types_array); // L'array era stato allocato
        config->rescuer_types_array = NULL;
        return false;
    }

    sprintf(log_msg_buffer, "Completamento parsing: Letti %d tipi di soccorritori validi. Totale gemelli digitali da creare: %d.", config->num_rescuer_types, config->total_digital_twins_to_create);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    return true;
}

*/