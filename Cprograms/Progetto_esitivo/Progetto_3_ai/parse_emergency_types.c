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

// Funzione helper che pulisce la stringa e restituisce il puntatore all'inizio dei dati utili
static char* clean_and_get_line_start(char* s) {
    if (!s) return NULL;
    // Rimuovi newline
    s[strcspn(s, "\n")] = 0;
    // Salta spazi iniziali
    while(isspace((unsigned char)*s)) s++;
    if (*s == 0) return s; // Stringa vuota o solo spazi
    // Rimuovi spazi finali
    char* end = s + strlen(s) - 1;
    while(end >= s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s; // Restituisce il puntatore all'inizio dei dati puliti
}


static rescuer_type_t* find_rescuer_type_in_array_pet(const char* name, const system_config_t* config) {
    for (int i = 0; i < config->num_rescuer_types; ++i) {
        // Confronta il nome cercato con il nome di ciascun tipo di soccorritore disponibile
        if (strcmp(config->rescuer_types_array[i].rescuer_type_name, name) == 0) {
            // Se trovato, restituisce un puntatore a quella struttura rescuer_type_t
            return &config->rescuer_types_array[i];
        }
    }
    return NULL; // Se non trovato dopo aver scansionato tutto l'array
}

bool parse_emergency_types(const char *filename, system_config_t *config) {
    // Buffer per costruire i messaggi di log formattati
    char log_msg_buffer[MAX_LINE_LENGTH + 250]; // Abbastanza grande per contenere una riga e testo aggiuntivo

    // 1. Validazione degli Input Iniziali
    if (!filename || !config) { // Controlla se i puntatori a filename e config sono validi
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false; // Fallimento
    }
    // Controlla se la configurazione dei soccorritori (necessaria per validare le richieste) è presente
    if (config->num_rescuer_types > 0 && !config->rescuer_types_array) {
        sprintf(log_msg_buffer, "Configurazione soccorritori mancante o corrotta (num_rescuer_types=%d ma array nullo).", config->num_rescuer_types);
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false; // Fallimento
    }

    // 2. Apertura del File
    sprintf(log_msg_buffer, "Tentativo apertura file: %s", filename);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    FILE *file = fopen(filename, "r"); // Apre il file in modalità lettura ("r")
    if (!file) { // Se fopen fallisce (restituisce NULL)
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno)); // strerror(errno) dà una descrizione testuale dell'errore di sistema
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        return false; // Fallimento
    }
    log_message(LOG_EVENT_FILE_PARSING, filename, "File aperto con successo.");

    // Variabili per il parsing
    char line[MAX_LINE_LENGTH]; // Buffer per leggere una riga alla volta
    int line_num_pass1 = 0;     // Contatore di righe per il primo passaggio
    int type_count = 0;         // Numero stimato di tipi di emergenza (dal primo passaggio)

    // 3. Primo Passaggio: Conteggio dei Tipi di Emergenza
    // Serve per sapere quanta memoria allocare per l'array `emergency_types_array`.
    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio primo passaggio: conteggio tipi emergenze.");
    while (fgets(line, sizeof(line), file)) { // Legge una riga fino a EOF o errore
        line_num_pass1++;
        // Crea una copia della riga per lavorarci senza modificare 'line' che potrebbe essere usata da altre funzioni
        char count_line_buffer[MAX_LINE_LENGTH];
        strncpy(count_line_buffer, line, MAX_LINE_LENGTH-1);
        count_line_buffer[MAX_LINE_LENGTH-1] = '\0'; // Assicura null-termination
        
        count_line_buffer[strcspn(count_line_buffer, "\n")] = 0; // Rimuove il newline
        trim_whitespace_pet(count_line_buffer); // Rimuove spazi bianchi iniziali/finali

        if (*count_line_buffer == '\0' || count_line_buffer[0] == '#') { // Ignora righe vuote o commenti
            continue;
        }
        // Controllo di formato molto basilare solo per il conteggio.
        // Se la riga sembra iniziare con un nome e una priorità, la contiamo.
        // La validazione completa avviene nel secondo passaggio.
        char temp_name[EMERGENCY_NAME_LENGTH];
        short temp_prio;
        if (sscanf(count_line_buffer, " [%[^]]] [%hd]", temp_name, &temp_prio) >= 2) {
            type_count++;
        }
    }
    sprintf(log_msg_buffer, "Primo passaggio completato: Trovati %d potenziali tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    // 4. Gestione File Vuoto o Senza Voci Valide
    if (type_count == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Nessun tipo di emergenza trovato. Parsing completato (file vuoto o senza voci valide).");
        config->num_emergency_types = 0;
        config->emergency_types_array = NULL; // Assicura che sia NULL
        fclose(file); // Chiude il file
        return true; // Non è un errore fatale, il file potrebbe essere legittimamente vuoto.
    }

    // 5. Allocazione Memoria per l'Array dei Tipi di Emergenza
    config->emergency_types_array = (emergency_type_t *)malloc(type_count * sizeof(emergency_type_t));
    if (!config->emergency_types_array) { // Se malloc fallisce
        sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d emergency_type_t: %s", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        fclose(file);
        return false; // Errore fatale
    }
    sprintf(log_msg_buffer, "Allocato array per %d tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    config->num_emergency_types = 0; // Inizializza il contatore effettivo a 0 (sarà incrementato nel secondo pass)

    // 6. Secondo Passaggio: Estrazione e Popolamento dei Dati
    rewind(file); // Riporta il puntatore del file all'inizio per rileggere il file
    int line_num_pass2 = 0;     // Contatore di righe per il secondo passaggio
    int current_type_idx = 0;   // Indice per l'array `emergency_types_array`

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio secondo passaggio: estrazione dati emergenze.");
    // Legge il file riga per riga, finché ci sono righe e non abbiamo riempito l'array allocato
    // (current_type_idx < type_count è una sicurezza aggiuntiva)
    
    while (fgets(line, sizeof(line), file) && current_type_idx < type_count) {
        line_num_pass2++; // Incrementa il numero di riga per il secondo passaggio

        // Conserva una copia della riga originale per il logging degli errori
        char original_line_for_log[MAX_LINE_LENGTH];
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH - 1);
        original_line_for_log[MAX_LINE_LENGTH - 1] = '\0';
        // Rimuovi newline dalla copia per il log, se presente, per una visualizzazione più pulita
        if (strchr(original_line_for_log, '\n')) {
            *strchr(original_line_for_log, '\n') = '\0';
        }
        
        // Lavora su una copia per il parsing, per non alterare `line` con strtok se usata altrove
        char temp_line_for_parsing[MAX_LINE_LENGTH];
        strcpy(temp_line_for_parsing, line);

        // Pulisce la linea e ottiene un puntatore all'inizio dei dati significativi.
        // Questo puntatore verrà fatto avanzare.
        char* current_line_ptr = clean_and_get_line_start(temp_line_for_parsing);

        // Ignora righe vuote (dopo la pulizia) o righe di commento
        if (*current_line_ptr == '\0' || current_line_ptr[0] == '#') {
            continue; // Passa alla prossima iterazione del while (prossima riga)
        }

        // Variabili per contenere i dati estratti dalla riga corrente
        char name_buffer[EMERGENCY_NAME_LENGTH];
        short priority_val;
        char rescuers_full_str[MAX_LINE_LENGTH] = ""; // Inizializza a stringa vuota

        // --- Estrai Nome Emergenza: formato "[<name>]" ---
        if (*current_line_ptr != '[') { // La riga deve iniziare con '['
            goto format_error_pet_loop; // Salta all'etichetta di errore di formato per questa riga
        }
        current_line_ptr++; // Avanza il puntatore oltre '['
        
        char *name_end = strchr(current_line_ptr, ']'); // Trova la ']' di chiusura del nome
        if (!name_end || (name_end == current_line_ptr) || (name_end - current_line_ptr >= EMERGENCY_NAME_LENGTH)) {
            // Se ']' non trovata, o nome vuoto, o nome troppo lungo
            goto format_error_pet_loop;
        }
        strncpy(name_buffer, current_line_ptr, name_end - current_line_ptr); // Copia il nome
        name_buffer[name_end - current_line_ptr] = '\0'; // Aggiunge il terminatore nullo
        current_line_ptr = name_end + 1; // Avanza il puntatore oltre ']'

        // --- Estrai Priorità: formato "[<priority>]" ---
        while(isspace((unsigned char)*current_line_ptr)) current_line_ptr++; // Salta eventuali spazi prima di '[' della priorità
        
        if (*current_line_ptr != '[') { // Deve esserci '[' per la priorità
            goto format_error_pet_loop;
        }
        current_line_ptr++; // Avanza oltre '['
        
        char *prio_end = strchr(current_line_ptr, ']'); // Trova la ']' di chiusura della priorità
        if (!prio_end) { // Se ']' non trovata
            goto format_error_pet_loop;
        }

        char prio_str_buf[10]; // Buffer temporaneo per la stringa della priorità
        if((prio_end == current_line_ptr) || ((prio_end - current_line_ptr) >= sizeof(prio_str_buf))) {
            // Se priorità vuota o stringa troppo lunga per il buffer
            goto format_error_pet_loop;
        }
        strncpy(prio_str_buf, current_line_ptr, prio_end - current_line_ptr); // Copia la stringa della priorità
        prio_str_buf[prio_end - current_line_ptr] = '\0'; // Aggiunge il terminatore nullo
        
        if(sscanf(prio_str_buf, "%hd", &priority_val) != 1) { // Converte la stringa in short
            goto format_error_pet_loop; // Se la conversione fallisce
        }
        current_line_ptr = prio_end + 1; // Avanza il puntatore oltre ']'
        




        // --- Estrai Stringa dei Soccorritori (tutto il resto della riga) ---
        while(isspace((unsigned char)*current_line_ptr)) current_line_ptr++; // Salta eventuali spazi prima della stringa dei soccorritori
        
        if(*current_line_ptr != '\0') { // Se c'è ancora testo sulla riga
            strncpy(rescuers_full_str, current_line_ptr, sizeof(rescuers_full_str)-1);
            rescuers_full_str[sizeof(rescuers_full_str)-1] = '\0'; // Assicura null-termination
        }

        // --- Validazione dei valori principali estratti (Nome, Priorità, Stringa Soccorritori) ---
        if (priority_val < PRIORITY_LOW || priority_val > PRIORITY_HIGH) {
            sprintf(log_msg_buffer, "Riga %d: Priorità emergenza (%hd) non valida. Valori ammessi: %d, %d, %d. Riga: '%s'. Riga ignorata.", line_num_pass2, priority_val, PRIORITY_LOW, PRIORITY_MEDIUM, PRIORITY_HIGH, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga e passa alla successiva
        }
        // Il nome è già stato controllato per lunghezza e non vuoto durante l'estrazione.
        if (strlen(rescuers_full_str) == 0) {
            sprintf(log_msg_buffer, "Riga %d: Lista soccorritori mancante per emergenza '%s'. Riga: '%s'. Riga ignorata.", line_num_pass2, name_buffer, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga
        }

        // --- Parsing della stringa dei soccorritori (`rescuers_full_str`) ---
        int req_array_capacity = 5; // Capacità iniziale per l'array di richieste
        rescuer_request_t *requests_arr = malloc(req_array_capacity * sizeof(rescuer_request_t));
        if (!requests_arr) {
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per richieste soccorritori (emergenza '%s'): %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            // Questo è un errore fatale, dobbiamo liberare la memoria principale e uscire
            // (il goto error_cleanup_pet_fatal gestirà la pulizia dell'array principale)
            // Qui però non c'è un array principale da pulire per *questa* iterazione fallita
            // La pulizia dell'array principale avviene fuori dal loop o in error_cleanup_pet_fatal
            // Se vogliamo uscire subito:
            fclose(file); // Chiudi il file prima di uscire
             // Libera l'array principale se parzialmente popolato
            for (int i = 0; i < current_type_idx; ++i) {
                if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
                if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
            }
            if(config->emergency_types_array) free(config->emergency_types_array);
            config->emergency_types_array = NULL;
            config->num_emergency_types = 0;
            return false;
        }
        int actual_req_count = 0;
        char *current_rescuer_token;
        // strtok_r modifica la stringa, quindi passiamo rescuers_full_str che è una copia
        char *rest_of_rescuers_str = rescuers_full_str; 
        char *saveptr_token;
        
        while ((current_rescuer_token = strtok_r(rest_of_rescuers_str, ";", &saveptr_token)) != NULL) {
            rest_of_rescuers_str = NULL; // Per le chiamate successive a strtok_r
            
            char token_copy_for_trim[MAX_LINE_LENGTH]; // strtok_r potrebbe restituire puntatori dentro rescuers_full_str
            strncpy(token_copy_for_trim, current_rescuer_token, MAX_LINE_LENGTH-1);
            token_copy_for_trim[MAX_LINE_LENGTH-1] = '\0';
            char* trimmed_token = clean_and_get_line_start(token_copy_for_trim);


            if(strlen(trimmed_token) == 0) continue; // Ignora token vuoti (es. da "item1;;item2")

            char rt_name_buf[RESCUER_NAME_MAX_LEN];
            int num_needed_val, time_needed_val;

            if (sscanf(trimmed_token, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Formato richiesta soccorritore non valido ('%s'). Richiesta ignorata.", line_num_pass2, name_buffer, trimmed_token);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora questa singola richiesta malformata
            }
            // Non serve un ulteriore trim su rt_name_buf se sscanf con %[^:] ha funzionato correttamente

            if (num_needed_val <= 0 || time_needed_val <= 0 || strlen(rt_name_buf) == 0) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Valori richiesta soccorritore non validi (num=%d, time=%d, nome='%s'). Richiesta ignorata.", line_num_pass2, name_buffer, num_needed_val, time_needed_val, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora
            }

            rescuer_type_t *found_rt = find_rescuer_type_in_array_pet(rt_name_buf, config);
            if (!found_rt) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Tipo soccorritore '%s' non trovato. Richiesta ignorata.", line_num_pass2, name_buffer, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora
            }

            if (actual_req_count >= req_array_capacity) {
                req_array_capacity *= 2;
                rescuer_request_t *temp_req_arr = realloc(requests_arr, req_array_capacity * sizeof(rescuer_request_t));
                if (!temp_req_arr) {
                    sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Fallimento riallocazione richieste soccorritori: %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
                    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                    free(requests_arr);
                    // Errore fatale, pulizia e uscita
                    fclose(file);
                    for (int i = 0; i < current_type_idx; ++i) {
                        if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
                        if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
                    }
                    if(config->emergency_types_array) free(config->emergency_types_array);
                    config->emergency_types_array = NULL; config->num_emergency_types = 0;
                    return false;
                }
                requests_arr = temp_req_arr;
            }
            requests_arr[actual_req_count].type = found_rt;
            requests_arr[actual_req_count].required_count = num_needed_val;
            requests_arr[actual_req_count].time_to_manage = time_needed_val;
            actual_req_count++;
        } // Fine loop strtok_r

        if (actual_req_count == 0) {
            sprintf(log_msg_buffer, "Riga %d: Nessuna richiesta soccorritore valida trovata per emergenza '%s' da stringa '%s'. Riga ignorata.", line_num_pass2, name_buffer, rescuers_full_str);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr); // requests_arr era stato allocato
            continue; // Ignora questa emergenza e passa alla prossima riga
        }
        
        // --- Popolamento della struttura emergency_type_t ---
        emergency_type_t *current_et_ptr = &config->emergency_types_array[current_type_idx];
        
        current_et_ptr->emergency_desc = strdup(name_buffer);
        if (!current_et_ptr->emergency_desc) {
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per nome emergenza '%s': %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr);
            // Errore fatale, pulizia e uscita
            fclose(file);
            for (int i = 0; i < current_type_idx; ++i) { // current_type_idx non è stato ancora incrementato per questa entry fallita
                if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
                if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
            }
            if(config->emergency_types_array) free(config->emergency_types_array);
            config->emergency_types_array = NULL; config->num_emergency_types = 0;
            return false;
        }
        current_et_ptr->priority = priority_val;
        current_et_ptr->rescuers = requests_arr; // Assegna l'array di richieste
        current_et_ptr->rescuers_req_number = actual_req_count;

        // Log del tipo di emergenza estratto con successo
        sprintf(log_msg_buffer, "Riga %d: Estratta emergenza: Nome='%s', Priorità=%hd, N.Richieste Socc.=%d",
                line_num_pass2, current_et_ptr->emergency_desc, current_et_ptr->priority, current_et_ptr->rescuers_req_number);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        for(int k=0; k < actual_req_count; ++k) { // Log dettagliato di ogni richiesta
            sprintf(log_msg_buffer, "    Richiesta %d: TipoSocc='%s', NumRichiesti=%d, TempoGestione=%d sec", k+1, requests_arr[k].type->rescuer_type_name, requests_arr[k].required_count, requests_arr[k].time_to_manage);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        }
        
        current_type_idx++; // Incrementa l'indice per il prossimo tipo di emergenza valido
        continue; // Passa alla prossima riga del file

    // Etichetta per errori di formato non fatali che invalidano solo la riga corrente
    format_error_pet_loop:
        sprintf(log_msg_buffer, "Riga %d: Errore formato: '%s'. Riga ignorata.", line_num_pass2, original_line_for_log);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        // Non c'è bisogno di liberare requests_arr qui perché l'errore è avvenuto prima della sua allocazione
        // o la sua gestione è interna al blocco try-catch implicito del parsing delle richieste.
        // Se `requests_arr` fosse stato allocato e poi si verificasse un errore qui, andrebbe liberato.
        // Ma gli errori di formato principali avvengono prima di allocare `requests_arr`.
        continue; // Salta al prossimo ciclo del while (prossima riga)
    } // --- Fine del ciclo while (fgets) ---

    // Imposta il numero effettivo di tipi di emergenza validi letti
    config->num_emergency_types = current_type_idx;

    fclose(file); // Chiude il file

    // Controllo finale: se non abbiamo letto nessun tipo valido nel secondo passaggio
    // nonostante il primo passaggio ne avesse stimati alcuni, è un problema.
    if (config->num_emergency_types == 0 && type_count > 0) {
        sprintf(log_msg_buffer, "Nessun tipo di emergenza valido estratto nel secondo passaggio, nonostante %d potenziali candidati dal primo passaggio. Parsing considerato fallito.", type_count);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        if (config->emergency_types_array) free(config->emergency_types_array); // L'array era stato allocato
        config->emergency_types_array = NULL;
        return false; // Fallimento
    }

    // Log di completamento del parsing
    sprintf(log_msg_buffer, "Completamento parsing: Letti %d tipi di emergenze validi.", config->num_emergency_types);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    return true; // Successo

// Etichetta per errori fatali (solitamente allocazione di memoria) che interrompono l'intero parsing.
error_cleanup_pet_fatal:
    // Libera la memoria allocata finora per i tipi di emergenza e le loro sotto-strutture.
    // `current_type_idx` indica quanti tipi sono stati *completamente e validamente* aggiunti.
    // Le risorse per il tipo *corrente* che ha causato l'errore potrebbero non essere in `config->emergency_types_array`
    // o potrebbero essere parzialmente allocate (es. `requests_arr` ma non `emergency_desc`).
    // Questa cleanup è una best-effort. La funzione `free_system_config` farà una pulizia più completa.
    for (int i = 0; i < current_type_idx; ++i) {
        if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
        if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
    }   
    if (config->emergency_types_array) free(config->emergency_types_array);
    config->emergency_types_array = NULL;
    config->num_emergency_types = 0;
    if(file) fclose(file); // Assicura che il file sia chiuso
    // Non è necessario loggare qui perché l'errore specifico è già stato loggato prima del goto.
    return false; // Fallimento
}






/*

bool parse_emergency_types(const char *filename, system_config_t *config) {
    char log_msg_buffer[MAX_LINE_LENGTH + 250];

    if (!filename || !config) { // Controlla se i puntatori a filename e config sono validi
        sprintf(log_msg_buffer, "Tentativo di parsing con argomenti nulli (filename o config).");
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false; // Fallimento
    }
    // Controlla se la configurazione dei soccorritori (necessaria per validare le richieste) è presente
    if (config->num_rescuer_types > 0 && !config->rescuer_types_array) {
        sprintf(log_msg_buffer, "Configurazione soccorritori mancante o corrotta (num_rescuer_types=%d ma array nullo).", config->num_rescuer_types);
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false; // Fallimento
    }


    sprintf(log_msg_buffer, "Tentativo apertura file: %s", filename);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    FILE *file = fopen(filename, "r"); // Apre il file in modalità lettura ("r")
    if (!file) { // Se fopen fallisce (restituisce NULL)
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno)); // strerror(errno) dà una descrizione testuale dell'errore di sistema
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        return false; // Fallimento
    }
    log_message(LOG_EVENT_FILE_PARSING, filename, "File aperto con successo.");

    // Variabili per il parsing
    char line[MAX_LINE_LENGTH]; // Buffer per leggere una riga alla volta
    int line_num_pass1 = 0;     // Contatore di righe per il primo passaggio
    int type_count = 0;         // Numero stimato di tipi di emergenza (dal primo passaggio)

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio primo passaggio: conteggio tipi emergenze.");
    
    while (fgets(line, sizeof(line), file)) {
        line_num_pass1++;
        // Crea una copia della riga per lavorarci senza modificare 'line' che potrebbe essere usata da altre funzioni
        char count_line_buffer[MAX_LINE_LENGTH];
        strncpy(count_line_buffer, line, MAX_LINE_LENGTH-1);
        count_line_buffer[MAX_LINE_LENGTH-1] = '\0'; // Assicura null-termination
        
        count_line_buffer[strcspn(count_line_buffer, "\n")] = 0; // Rimuove il newline
        trim_whitespace_pet(count_line_buffer); // Rimuove spazi bianchi iniziali/finali

        if (*count_line_buffer == '\0' || count_line_buffer[0] == '#') {
            continue;
        }

        // Controllo di formato molto basilare solo per il conteggio.
        // Se la riga sembra iniziare con un nome e una priorità, la contiamo.
        // La validazione completa avviene nel secondo passaggio.
        char temp_name[EMERGENCY_NAME_LENGTH];
        short temp_prio;
        if (sscanf(count_line_buffer, " [%[^]]] [%hd]", temp_name, &temp_prio) >= 2) {
            type_count++;
        }
    }
    sprintf(log_msg_buffer, "Primo passaggio completato: Trovati %d potenziali tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);

    if (type_count == 0) {
        log_message(LOG_EVENT_FILE_PARSING, filename, "Nessun tipo di emergenza trovato. Parsing completato (file vuoto o senza voci valide).");
        config->num_emergency_types = 0;
        config->emergency_types_array = NULL; // Assicura che sia NULL
        fclose(file); // Chiude il file
        return true; // Non è un errore fatale, il file potrebbe essere legittimamente vuoto.
    }

    // 5. Allocazione Memoria per l'Array dei Tipi di Emergenza
    config->emergency_types_array = (emergency_type_t *)malloc(type_count * sizeof(emergency_type_t));
    if (!config->emergency_types_array) { // Se malloc fallisce
        sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d emergency_type_t: %s", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        fclose(file);
        return false; // Errore fatale
    }

    sprintf(log_msg_buffer, "Allocato array per %d tipi di emergenze.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    config->num_emergency_types = 0; // Inizializza il contatore effettivo a 0 (sarà incrementato nel secondo pass)

    // 6. Secondo Passaggio: Estrazione e Popolamento dei Dati
    rewind(file); // Riporta il puntatore del file all'inizio per rileggere il file
    int line_num_pass2 = 0;     // Contatore di righe per il secondo passaggio
    int current_type_idx = 0;   // Indice per l'array `emergency_types_array`

    log_message(LOG_EVENT_FILE_PARSING, filename, "Inizio secondo passaggio: estrazione dati emergenze.");
    // Legge il file riga per riga, finché ci sono righe e non abbiamo riempito l'array allocato
    // (current_type_idx < type_count è una sicurezza aggiuntiva)

    while (fgets(line, sizeof(line), file) && current_type_idx < type_count) {
        line_num_pass2++;
        // Conserva una copia della riga originale per il logging degli errori
        char original_line_for_log[MAX_LINE_LENGTH];
        strncpy(original_line_for_log, line, MAX_LINE_LENGTH-1);
        original_line_for_log[MAX_LINE_LENGTH-1] = '\0';
        // Rimuovi newline dalla copia per il log, se presente
        if (strchr(original_line_for_log, '\n')) *strchr(original_line_for_log, '\n') = '\0';
        
        // Lavora su una copia per il parsing, per non alterare `line` con strtok se usata altrove
        char temp_line_for_parsing[MAX_LINE_LENGTH];
        strcpy(temp_line_for_parsing, line);
        char* current_line_ptr = temp_line_for_parsing; // Puntatore per scorrere la linea
        current_line_ptr[strcspn(current_line_ptr, "\n")] = 0; // Rimuovi newline
        trim_whitespace_pet(current_line_ptr); // Rimuovi spazi esterni

        if (*current_line_ptr == '\0' || current_line_ptr[0] == '#') {
            continue;
        }

        // Variabili per contenere i dati estratti dalla riga
        char name_buffer[EMERGENCY_NAME_LENGTH];
        short priority_val;
        char rescuers_full_str[MAX_LINE_LENGTH] = ""; // Inizializza a stringa vuota

        // Parsing manuale più robusto della riga (invece di un singolo sscanf complesso)
        char *parser_ptr = current_line_ptr; // Puntatore ausiliario per il parsing

        // Estrai Nome Emergenza: "[<name>]"
        if (*parser_ptr != '[') { goto format_error_pet; } // Deve iniziare con '['
        parser_ptr++; // Salta '['
        char *name_end = strchr(parser_ptr, ']'); // Trova la ']' corrispondente
        if (!name_end || (name_end - parser_ptr) == 0 || (name_end - parser_ptr) >= EMERGENCY_NAME_LENGTH) { goto format_error_pet; }
        strncpy(name_buffer, parser_ptr, name_end - parser_ptr); // Copia il nome
        name_buffer[name_end - parser_ptr] = '\0'; // Null-termina
        parser_ptr = name_end + 1; // Avanza il puntatore dopo ']'

        // Estrai Priorità: "[<priority>]"
        while(isspace((unsigned char)*parser_ptr)) parser_ptr++; // Salta spazi intermedi
        if (*parser_ptr != '[') { goto format_error_pet; }
        parser_ptr++; // Salta '['
        char *prio_end = strchr(parser_ptr, ']'); // Trova la ']' corrispondente
        if (!prio_end) { goto format_error_pet; }
        char prio_str_buf[10]; // Buffer per la stringa della priorità
        if((prio_end - parser_ptr) == 0 || (prio_end - parser_ptr) >= sizeof(prio_str_buf)) { goto format_error_pet; } // Lunghezza valida
        strncpy(prio_str_buf, parser_ptr, prio_end - parser_ptr); // Copia la stringa della priorità
        prio_str_buf[prio_end - parser_ptr] = '\0'; // Null-termina
        if(sscanf(prio_str_buf, "%hd", &priority_val) != 1) { goto format_error_pet; } // Converte in short
        parser_ptr = prio_end + 1; // Avanza il puntatore
        
        // Estrai Stringa dei Soccorritori (tutto il resto della riga)
        while(isspace((unsigned char)*parser_ptr)) parser_ptr++; // Salta spazi
        if(*parser_ptr != '\0') { // Se c'è ancora qualcosa sulla riga
            strncpy(rescuers_full_str, parser_ptr, sizeof(rescuers_full_str)-1);
            rescuers_full_str[sizeof(rescuers_full_str)-1] = '\0'; // Assicura null-termination
        }

        // Validazione dei valori estratti
        if (priority_val < PRIORITY_LOW || priority_val > PRIORITY_HIGH) {
            sprintf(log_msg_buffer, "Riga %d: Priorità emergenza (%hd) non valida. Valori ammessi: %d, %d, %d. Riga: '%s'. Riga ignorata.", line_num_pass2, priority_val, PRIORITY_LOW, PRIORITY_MEDIUM, PRIORITY_HIGH, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga e passa alla successiva
        }

        if (strlen(rescuers_full_str) == 0) { // La specifica implica che ci siano sempre richieste
            sprintf(log_msg_buffer, "Riga %d: Lista soccorritori mancante per emergenza '%s'. Riga: '%s'. Riga ignorata.", line_num_pass2, name_buffer, original_line_for_log);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            continue; // Ignora questa riga
        }

        // --- Parsing della stringa dei soccorritori (`rescuers_full_str`) ---
        // Esempio: "Pompieri:1,5;Ambulanza:1,2;"
        int req_array_capacity = 5; // Capacità iniziale per l'array di richieste
        rescuer_request_t *requests_arr = malloc(req_array_capacity * sizeof(rescuer_request_t));
        if (!requests_arr) { // Se malloc fallisce
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per richieste soccorritori (emergenza '%s'): %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            goto error_cleanup_pet_fatal; // Errore fatale, esce dalla funzione
        }

        int actual_req_count = 0; // Contatore effettivo delle richieste valide per questa emergenza
        char *current_rescuer_token; // Puntatore al token corrente (es. "Pompieri:1,5")
        char *rest_of_rescuers_str = rescuers_full_str; // strtok_r modifica la stringa, quindi usa la copia
        char *saveptr_token; // Puntatore per strtok_r (thread-safe)

        // Divide la stringa `rescuers_full_str` usando ';' come delimitatore
        while ((current_rescuer_token = strtok_r(rest_of_rescuers_str, ";", &saveptr_token)) != NULL) {
            rest_of_rescuers_str = NULL; // Per le chiamate successive a strtok_r sulla stessa stringa
            trim_whitespace_pet(current_rescuer_token); // Pulisce il token
            if(strlen(current_rescuer_token) == 0) continue; // Ignora token vuoti (es. se c'è " ;; ")
            
            // Variabili per il token corrente
            char rt_name_buf[RESCUER_NAME_MAX_LEN];
            int num_needed_val, time_needed_val;

            // Parsa il token: "<rescuertype>:<number>,<time_in_secs>"
            if (sscanf(current_rescuer_token, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Formato richiesta soccorritore non valido ('%s'). Richiesta ignorata.", line_num_pass2, name_buffer, current_rescuer_token);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora questa singola richiesta malformata, ma continua con le altre della stessa emergenza
            }
            trim_whitespace_pet(rt_name_buf); // Pulisce il nome del tipo di soccorritore estratto

            // Validazione dei dati della richiesta
            if (num_needed_val <= 0 || time_needed_val <= 0 || strlen(rt_name_buf) == 0) {
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Valori richiesta soccorritore non validi (num=%d, time=%d, nome='%s'). Richiesta ignorata.", line_num_pass2, name_buffer, num_needed_val, time_needed_val, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora
            }

            // Cerca il tipo di soccorritore nell'array dei tipi disponibili
            rescuer_type_t *found_rt = find_rescuer_type_in_array_pet(rt_name_buf, config);
            if (!found_rt) { // Se il tipo non esiste
                sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Tipo soccorritore '%s' non trovato nella configurazione dei soccorritori. Richiesta ignorata.", line_num_pass2, name_buffer, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                continue; // Ignora
            }

            // Se l'array `requests_arr` è pieno, rialloca per più spazio
            if (actual_req_count >= req_array_capacity) {
                req_array_capacity *= 2; // Raddoppia la capacità
                rescuer_request_t *temp_req_arr = realloc(requests_arr, req_array_capacity * sizeof(rescuer_request_t));
                if (!temp_req_arr) { // Se realloc fallisce
                    sprintf(log_msg_buffer, "Riga %d, Emergenza '%s': Fallimento riallocazione memoria per richieste soccorritori: %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
                    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
                    free(requests_arr); // Libera l'array parzialmente popolato
                    goto error_cleanup_pet_fatal; // Errore fatale
                }
                requests_arr = temp_req_arr; // Aggiorna il puntatore all'array riallocato
            }
            // Popola la struttura rescuer_request_t
            requests_arr[actual_req_count].type = found_rt; // Puntatore al tipo di soccorritore trovato
            requests_arr[actual_req_count].required_count = num_needed_val;
            requests_arr[actual_req_count].time_to_manage = time_needed_val;
            actual_req_count++; // Incrementa il contatore delle richieste valide per questa emergenza
        } // Fine loop strtok_r per le richieste di soccorritori

        // Se, dopo aver processato la stringa dei soccorritori, non ne abbiamo trovata nessuna valida,
        // l'intera riga del tipo di emergenza viene considerata non valida.

        if (actual_req_count == 0) {
            sprintf(log_msg_buffer, "Riga %d: Nessuna richiesta soccorritore valida trovata per emergenza '%s' dalla stringa '%s'. Riga ignorata.", line_num_pass2, name_buffer, rescuers_full_str);
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr); // `requests_arr` era stato allocato, quindi va liberato
            continue; // Passa alla prossima riga del file
        }
        
        // --- Fine Parsing Stringa Soccorritori ---

        // Se tutto è andato bene finora, popola la struttura emergency_type_t
        emergency_type_t *current_et_ptr = &config->emergency_types_array[current_type_idx];

        current_et_ptr->emergency_desc = strdup(name_buffer); // Alloca memoria e copia il nome dell'emergenza
        if (!current_et_ptr->emergency_desc) { // Se strdup fallisce
            sprintf(log_msg_buffer, "Riga %d: Fallimento allocazione memoria per nome emergenza '%s': %s. Parsing interrotto.", line_num_pass2, name_buffer, strerror(errno));
            log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
            free(requests_arr); // Libera l'array di richieste per questa emergenza che non verrà aggiunta
            goto error_cleanup_pet_fatal; // Errore fatale
        }
        current_et_ptr->priority = priority_val;
        current_et_ptr->rescuers = requests_arr; // Assegna l'array di richieste (già allocato e popolato)
        current_et_ptr->rescuers_req_number = actual_req_count; // Numero di richieste valide

        // Log del tipo di emergenza estratto con successo
        sprintf(log_msg_buffer, "Riga %d: Estratta emergenza: Nome='%s', Priorità=%hd, N.Richieste Socc.=%d",
            line_num_pass2, current_et_ptr->emergency_desc, current_et_ptr->priority, current_et_ptr->rescuers_req_number);
    log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    // Log dettagliato di ogni richiesta per questa emergenza (opzionale ma utile)
    for(int k=0; k < actual_req_count; ++k) {
        sprintf(log_msg_buffer, "    Richiesta %d: TipoSocc='%s', NumRichiesti=%d, TempoGestione=%d sec", k+1, requests_arr[k].type->rescuer_type_name, requests_arr[k].required_count, requests_arr[k].time_to_manage);
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
    }
        
        current_type_idx++; // Incrementa l'indice per il prossimo tipo di emergenza valido
        continue; // Passa alla prossima riga del file

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

*/

