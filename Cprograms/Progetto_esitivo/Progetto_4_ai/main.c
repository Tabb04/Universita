#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h> // Per future implementazioni multithread
#include <mqueue.h>  // Per code di messaggi POSIX
#include <fcntl.h>   // Per O_flags in mq_open
#include <sys/stat.h> // Per mode in mq_open
#include <errno.h>
#include <math.h>
#include <time.h>

#include "config1.h"
#include "data_types.h"
#include "logger.h"
#include "parser.h"
#include "Syscalls_3_progetto.h" 

// --- Variabili Globali (da considerare attentamente per il multithreading) ---
system_config_t global_system_config;
rescuer_digital_twin_t *global_digital_twins_array = NULL;
int total_digital_twins_created = 0; // Contatore per ID univoci e verifica

// Potrebbe servire un ID univoco per le emergenze
long next_emergency_id = 1; // Da proteggere con mutex se usato da più thread

typedef struct emergency_node {
    emergency_t data;
    struct emergency_node *next;
} emergency_node_t;

typedef struct {
    int thread_id_for_log; // L'ID che useremo per il logging
    // Potresti aggiungere altri puntatori o dati qui se necessario in futuro,
    // ad esempio un puntatore a una porzione specifica di dati che questo thread deve processare.
} thread_handler_args_t; // Rinominata per chiarezza rispetto a un generico thread_args_t

emergency_node_t *waiting_emergencies_list_head = NULL; // Testa della lista
emergency_node_t *waiting_emergencies_list_tail = NULL; // Coda della lista per aggiunte O(1)


mtx_t digital_twins_array_mutex;
mtx_t waiting_emergencies_mutex; // Mutex per proteggere la lista
cnd_t new_emergency_condition;   // Variabile di condizione per segnalare nuove emergenze


// --- Variabile per la terminazione controllata ---
volatile bool shutdown_flag = false; // Impostata a true per terminare i thread

// --- Descrittore della coda di messaggi POSIX ---
mqd_t global_mq_descriptor = (mqd_t)-1; // Inizializza a un valore non valido



// Funzione di pulizia generale da chiamare prima di uscire in caso di errore parziale
void cleanup_before_exit(bool config_loaded, bool digital_twins_loaded, bool mq_loaded, bool waiting_sync_loaded, bool dt_mutex_loaded) {
    log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Inizio pulizia generale.");

    if(mq_loaded && global_mq_descriptor != (mqd_t)-1){
        mq_close(global_mq_descriptor);
        log_message(LOG_EVENT_MESSAGE_QUEUE, "CLEANUP", "Coda messaggi POSIX chiusa.");
    }
    
    if (digital_twins_loaded && global_digital_twins_array) {
        // Se i digital twin avessero memoria interna da liberare (non in questo caso), andrebbe fatto qui.
        free(global_digital_twins_array);
        global_digital_twins_array = NULL;
        log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Array gemelli digitali deallocato.");
    }
    if (config_loaded) {
        free_system_config(&global_system_config);
    }

    if (waiting_sync_loaded) {
        mtx_destroy(&waiting_emergencies_mutex);
        cnd_destroy(&new_emergency_condition);
        log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Primitive di sincronizzazione distrutte.");
    }

    if (dt_mutex_loaded) { // NUOVO BLOCCO
        mtx_destroy(&digital_twins_array_mutex);
        log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Mutex per array gemelli digitali distrutto.");
    }

    log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Pulizia completata.");
    close_logger();
}

// In main.c, PRIMA della funzione main()

// Funzione eseguita dal thread listener della coda di messaggi
int message_queue_listener_thread_func(void *arg) {
    (void)arg; // Argomento non usato in questo esempio
    emergency_request_t received_request;
    char log_msg_buffer[LINE_LENGTH + 250];
    unsigned int priority; // Per mq_receive

    log_message(LOG_EVENT_GENERAL_INFO, "MQ_LISTENER_THREAD", "Thread listener coda messaggi avviato.");

    while (!shutdown_flag) {
        // Attendi un messaggio sulla coda
        // mq_receive è bloccante. Potremmo voler usare mq_timedreceive per permettere
        // un controllo periodico di shutdown_flag, ma per ora usiamo la bloccante.
        ssize_t bytes_read = mq_receive(global_mq_descriptor, (char *)&received_request,
                                        sizeof(emergency_request_t), &priority);

        if (shutdown_flag) { // Controlla subito dopo il risveglio
            break;
        }

        if (bytes_read == -1) {
            if (errno == EINTR && shutdown_flag) { // Interrotto da un segnale, e dobbiamo chiudere
                 log_message(LOG_EVENT_MESSAGE_QUEUE, "MQ_LISTENER_THREAD", "mq_receive interrotto da segnale e shutdown richiesto.");
                 break;
            }

            //per ora no
            if (errno == EAGAIN) { // Coda vuota con O_NONBLOCK (non il nostro caso ora) o timeout scaduto (con mq_timedreceive)
                continue; // Riprova se non stiamo chiudendo
            }
            sprintf(log_msg_buffer, "Errore in mq_receive: %s. Il thread listener terminerà.", strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MQ_LISTENER_THREAD", log_msg_buffer);
            // Potrebbe essere necessario un meccanismo per segnalare al main questo errore fatale del thread
            break; // Esci dal loop in caso di errore grave
        }

        if (bytes_read != sizeof(emergency_request_t)) {
            sprintf(log_msg_buffer, "Ricevuto messaggio di dimensione errata: %zd byte invece di %zu. Messaggio scartato.",
                    bytes_read, sizeof(emergency_request_t));
            log_message(LOG_EVENT_MESSAGE_QUEUE, "MQ_LISTENER_THREAD", log_msg_buffer);
            continue;
        }

        sprintf(log_msg_buffer, "Ricevuta richiesta emergenza: Tipo='%s', Pos=(%d,%d), Timestamp=%ld",
                received_request.emergency_name, received_request.x, received_request.y, (long)received_request.timestamp);
        // L'ID qui dovrebbe essere un ID univoco generato per questa richiesta ricevuta,
        // ma per ora usiamo N/A o un contatore semplice.
        log_message(LOG_EVENT_MESSAGE_QUEUE, "NEW_REQUEST", log_msg_buffer);

        // --- Validazione della richiesta e creazione di emergency_t ---
        emergency_type_t *found_type = NULL;
        for (int i = 0; i < global_system_config.emergency_type_num; ++i) {
            if (strcmp(global_system_config.emergency_types_array[i].emergency_desc, received_request.emergency_name) == 0) {
                found_type = &global_system_config.emergency_types_array[i];
                break;
            }
        }

        if (!found_type) {
            sprintf(log_msg_buffer, "Tipo emergenza '%s' non riconosciuto. Richiesta scartata.", received_request.emergency_name);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "INVALID_REQUEST", log_msg_buffer);
            continue;
        }

        if (received_request.x < 0 || received_request.x >= global_system_config.config_env.grid_width ||
            received_request.y < 0 || received_request.y >= global_system_config.config_env.grid_height) {
            sprintf(log_msg_buffer, "Coordinate emergenza (%d,%d) fuori dalla griglia (%dx%d) per tipo '%s'. Richiesta scartata.",
                    received_request.x, received_request.y,
                    global_system_config.config_env.grid_width, global_system_config.config_env.grid_height,
                    received_request.emergency_name);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "INVALID_REQUEST", log_msg_buffer);
            continue;
        }

        // Crea un nuovo nodo per la lista delle emergenze
        emergency_node_t *new_node = (emergency_node_t *)malloc(sizeof(emergency_node_t));
        if (!new_node) {
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per nuovo nodo emergenza: %s. Richiesta scartata.", strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MQ_LISTENER_THREAD", log_msg_buffer);
            continue; // Perdi questa emergenza, ma il thread continua
        }

        // Popola emergency_t nel nodo
        // next_emergency_id dovrebbe essere protetto da mutex se più thread lo modificano. Per ora è solo questo thread.
        // Per ora non assegno un ID all'emergenza, lo farà il gestore.
        // new_node->data.id = next_emergency_id++; // Esempio
        new_node->data.type = found_type;
        new_node->data.status = WAITING;
        new_node->data.x = received_request.x;
        new_node->data.y = received_request.y;
        new_node->data.time = time(NULL); // Timestamp di ricezione da parte del server
        new_node->data.resquer_cont = 0; // Inizialmente nessun soccorritore assegnato
        new_node->data.rescuer_dt = NULL; // Verrà allocato quando i soccorritori sono assegnati
        new_node->next = NULL;

        // Aggiungi il nodo alla coda interna (protetta da mutex)
        mtx_lock(&waiting_emergencies_mutex);
        if (waiting_emergencies_list_tail == NULL) { // Lista vuota
            waiting_emergencies_list_head = new_node;
            waiting_emergencies_list_tail = new_node;
        } else {
            waiting_emergencies_list_tail->next = new_node;
            waiting_emergencies_list_tail = new_node;
        }
        sprintf(log_msg_buffer, "Nuova emergenza (Tipo: %s, Pos: %d,%d) aggiunta alla coda interna in stato WAITING.",
                new_node->data.type->emergency_desc, new_node->data.x, new_node->data.y);
        log_message(LOG_EVENT_EMERGENCY_STATUS, "INTERNAL_QUEUE", log_msg_buffer); // Potrebbe servire un ID emergenza qui

        mtx_unlock(&waiting_emergencies_mutex);

        // Segnala ai thread gestori che c'è una nuova emergenza
        cnd_signal(&new_emergency_condition);
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MQ_LISTENER_THREAD", "Thread listener coda messaggi in terminazione.");
    return 0; // Valore di ritorno per thrd_join
}


static int compare_candidates(const void *a, const void *b) {

    rescuer_candidate_t *ca = (rescuer_candidate_t *)a;
    rescuer_candidate_t *cb = (rescuer_candidate_t *)b;
    if (ca->time_to_arrive < cb->time_to_arrive) return -1;
    if (ca->time_to_arrive > cb->time_to_arrive) return 1;
    // Se i tempi sono uguali, potremmo ordinare per ID per stabilità (opzionale)
    // if (ca->dt->id < cb->dt->id) return -1;
    // if (ca->dt->id > cb->dt->id) return 1;
    return 0;
}

typedef struct {
    rescuer_digital_twin_t *dt;
    double time_to_arrive;
} rescuer_candidate_t;


// --- Definizione e Inizializzazione del Mutex per i Gemelli Digitali ---
// (Da aggiungere alle variabili globali)
mtx_t digital_twins_array_mutex;

// --- Funzione per i Thread Gestori di Emergenze ---
int emergency_handler_thread_func(void *arg) {
    thread_handler_args_t *thread_args = (thread_handler_args_t *)arg;
    
    if(!thread_args){
        log_message(LOG_EVENT_GENERAL_ERROR, "HANDLER_THREAD", "Argomenti del thread nulli. Terminazione anomala.");
        return -1; // o un altro codice di errore
    }
    
    int thread_id_for_log = thread_args->thread_id_for_log;

    char log_msg_buffer[LINE_LENGTH + 300];
    emergency_node_t *current_emergency_node = NULL;

    sprintf(log_msg_buffer, "Thread Gestore Emergenze #%d avviato.", thread_id_for_log);
    log_message(LOG_EVENT_GENERAL_INFO, "HANDLER_THREAD", log_msg_buffer);

    while (!shutdown_flag) {
        // 1. Attendere e Prelevare un'Emergenza
        mtx_lock(&waiting_emergencies_mutex);

        while (waiting_emergencies_list_head == NULL && !shutdown_flag) {
            // Attendi finché non c'è un'emergenza o non arriva il segnale di shutdown
            // sprintf(log_msg_buffer, "Thread #%ld in attesa di emergenze...", thread_id);
            // log_message(LOG_EVENT_GENERAL_INFO, "HANDLER_THREAD", log_msg_buffer); // Log molto verboso
            cnd_wait(&new_emergency_condition, &waiting_emergencies_mutex);
        }

        if (shutdown_flag) {
            mtx_unlock(&waiting_emergencies_mutex);
            break; // Esci dal loop principale del thread
        }

        // Preleva l'emergenza dalla testa della lista
        current_emergency_node = waiting_emergencies_list_head;

        waiting_emergencies_list_head = waiting_emergencies_list_head->next;

        if (waiting_emergencies_list_head == NULL) { // La lista è diventata vuota
            waiting_emergencies_list_tail = NULL;
        }
        mtx_unlock(&waiting_emergencies_mutex);

        emergency_type_t *e_type = current_emergency_node->data.type; // Accesso diretto
        long emergency_log_id = (long)current_emergency_node->data.time; // Usiamo il timestamp come ID temporaneo per i log
        char emergency_id_str[40]; // Buffer per l'ID nei log
        snprintf(emergency_id_str, sizeof(emergency_id_str), "EMG_%ld", emergency_log_id);

        /*
        qui stava TODO
        */

        sprintf(log_msg_buffer, "Thread #%d: Processo Emergenza %s (Tipo: %s, Pos: %d,%d)",
        thread_id_for_log, emergency_id_str, e_type->emergency_desc,
        current_emergency_node->data.x, // Accesso diretto
        current_emergency_node->data.y); // Accesso diretto
        log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, log_msg_buffer);

        // 1. Calcolo Deadline (Tempo massimo per lo stato IN_PROGRESS)
        time_t deadline = 0;
        if (e_type->priority == MEDIA_PRIOR) {
            deadline = current_emergency_node->data.time + MEDIA_PRIOR_TIME; // Accesso diretto
        } else if (e_type->priority == ALTA_PRIOR) {
            deadline = current_emergency_node->data.time + ALTA_PRIOR_TIME; // Accesso diretto
        }

        if (deadline > 0) {
            sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Deadline calcolato: %ld",
                thread_id_for_log, emergency_id_str, (long)deadline);
            log_message(LOG_EVENT_GENERAL_INFO, emergency_id_str, log_msg_buffer);
        }

        // Strutture dati temporanee per la selezione dei soccorritori
        int total_rescuers_needed = 0;
        for (int i = 0; i < e_type->rescuer_required_number; ++i) {
            total_rescuers_needed += e_type->rescuers[i].required_count;
        }


        // Array per memorizzare i puntatori ai *migliori* candidati trovati
        rescuer_digital_twin_t **selected_rescuers = NULL;
        
        if (total_rescuers_needed > 0) {
            selected_rescuers = (rescuer_digital_twin_t **)calloc(total_rescuers_needed, sizeof(rescuer_digital_twin_t *));
            if (!selected_rescuers) {
                sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Fallimento allocazione memoria per array puntatori soccorritori.", thread_id_for_log, emergency_id_str);
                log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, log_msg_buffer);
                // Consideriamo questo come un timeout/cancellazione? Dipende dai requisiti.
                // Per ora, liberiamo il nodo e continuiamo
                current_emergency_node->data.status = CANCELED; // O TIMEOUT // Accesso diretto
                log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a CANCELED (malloc fallito)");
                free(current_emergency_node); // Non liberare rescuer_dt perchè selected_rescuers è fallito
                current_emergency_node = NULL;
                continue; // Vai alla prossima iterazione del while
            }
        } else {
            sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Nessun soccorritore richiesto nel tipo emergenza? Controllare config.", thread_id_for_log, emergency_id_str);
            log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, log_msg_buffer);
            current_emergency_node->data.status = CANCELED; // Accesso diretto
            log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a CANCELED (nessun soccorritore richiesto)");
            free(current_emergency_node);
            current_emergency_node = NULL;
            continue;
        }
        int current_selection_index = 0;
        bool assignment_possible = true;
        double max_time_to_arrive = 0.0; // Tempo massimo di arrivo tra tutti i soccorritori selezionati


        // 2. Ciclo per ogni tipo di soccorritore richiesto dall'emergenza
        for (int i = 0; i < e_type->rescuer_required_number; ++i) {
            rescuer_request_t *req = &e_type->rescuers[i]; // La richiesta corrente
            rescuer_type_t *required_type = req->type;
            int needed_count = req->required_count;

            sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Ricerca %d unità di tipo '%s'",
                    thread_id_for_log, emergency_id_str, needed_count, required_type->rescuer_type_name);
            log_message(LOG_EVENT_ASSIGNMENT, emergency_id_str, log_msg_buffer);

            // Lista temporanea per i candidati IDLE di questo tipo
            rescuer_candidate_t *candidates = (rescuer_candidate_t *)malloc(total_digital_twins_created * sizeof(rescuer_candidate_t));
            if(!candidates){
                sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Fallimento allocazione memoria per array candidati tipo '%s'.", thread_id_for_log, emergency_id_str, required_type->rescuer_type_name);
                log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, log_msg_buffer);
                assignment_possible = false;
                // Non posso uscire subito dal loop esterno, marco solo come fallito
                break; // Esci dal loop dei tipi richiesti
            }
            int candidate_count = 0;

            // --- Blocco Critico: Lettura stato soccorritori ---
            mtx_lock(&digital_twins_array_mutex);
            for (int k = 0; k < total_digital_twins_created; ++k) {
                rescuer_digital_twin_t *dt = &global_digital_twins_array[k];
                if (dt->rescuer == required_type && dt->status == IDLE) {
                    // Calcola tempo di arrivo
                    int distance = abs(dt->x - current_emergency_node->data.x) + abs(dt->y - current_emergency_node->data.y); // Accesso diretto
                    double time_arr = (dt->rescuer->speed > 0) ? ((double)distance / dt->rescuer->speed) : -1.0; // -1 indica errore/velocità 0

                    if(time_arr >= 0) { // Ignora se velocità è 0 o invalida
                        candidates[candidate_count].dt = dt;
                        candidates[candidate_count].time_to_arrive = time_arr;
                        candidate_count++;
                    } else {
                        sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Soccorritore ID %d ('%s') ha velocità <= 0, ignorato.",
                                thread_id_for_log, emergency_id_str, dt->id, dt->rescuer->rescuer_type_name);
                        log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, log_msg_buffer);
                    }
                }
            }
            mtx_unlock(&digital_twins_array_mutex);
            // --- Fine Blocco Critico ---

            if (candidate_count < needed_count) {
                sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Risorse insufficienti: trovati %d candidati IDLE per tipo '%s', ma necessari %d.",
                        thread_id_for_log, emergency_id_str, candidate_count, required_type->rescuer_type_name, needed_count);
                log_message(LOG_EVENT_ASSIGNMENT, emergency_id_str, log_msg_buffer);
                assignment_possible = false;
                free(candidates); // Libera la memoria dei candidati per questo tipo
                break; // Esci dal loop dei tipi richiesti
            }

            // Ordina i candidati per tempo di arrivo (usando qsort)
            qsort(candidates, candidate_count, sizeof(rescuer_candidate_t), compare_candidates);

            // Seleziona i primi 'needed_count' candidati e aggiungili all'array generale
            for (int sel = 0; sel < needed_count; ++sel) {
                if (current_selection_index < total_rescuers_needed) {
                    selected_rescuers[current_selection_index] = candidates[sel].dt; // Salva il puntatore al DT
                    // Aggiorna il tempo massimo di arrivo generale
                    if (candidates[sel].time_to_arrive > max_time_to_arrive) {
                        max_time_to_arrive = candidates[sel].time_to_arrive;
                    }
                    current_selection_index++;
                } else {
                    // Errore logico, non dovrebbe accadere
                    log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, "Errore logico: Indice selezione soccorritori ha superato il totale.");
                    assignment_possible = false;
                    break; // Esci dal loop di selezione
                }
            }
            free(candidates); // Libera la memoria dei candidati per questo tipo

            if (!assignment_possible) break; // Se c'è stato un errore logico sopra, esci
        } // Fine loop sui tipi di soccorritori richiesti

        
        // 3. Controllo finale: Assignment possibile E rispetto deadline?
        if (assignment_possible) {
            // Controllo deadline
            time_t now = time(NULL); // O potremmo usare current_emergency_node->data.time
            time_t estimated_arrival_time = now + (time_t)max_time_to_arrive;

            if (deadline > 0 && estimated_arrival_time > deadline) {
                sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - TIMEOUT: Tempo di arrivo stimato (%ld, max_wait=%.0f) supera deadline (%ld).",
                        thread_id_for_log, emergency_id_str, (long)estimated_arrival_time, max_time_to_arrive, (long)deadline);
                log_message(LOG_EVENT_TIMEOUT, emergency_id_str, log_msg_buffer);
                assignment_possible = false;
                current_emergency_node->data.status = TIMEOUT; // Accesso diretto
                log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a TIMEOUT (deadline non rispettato).");
            }
        }

        // Se l'assignment non è stato possibile (risorse o tempo)
        if (!assignment_possible) {
            free(selected_rescuers); // Libera l'array di puntatori (vuoto o parziale)
            // Controlla se lo stato non è già stato impostato a TIMEOUT/CANCELED per altri motivi
            if(current_emergency_node->data.status != TIMEOUT && current_emergency_node->data.status != CANCELED) { // Accesso diretto
                current_emergency_node->data.status = TIMEOUT; // O CANCELED se preferito per mancanza risorse // Accesso diretto
                log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a TIMEOUT (risorse insufficienti).");
            }
            free(current_emergency_node); // Libera il nodo
            current_emergency_node = NULL;
            continue; // Vai alla prossima emergenza
        }



        // 4. Assignment Effettivo (se tutto OK finora)
        sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Assegnazione confermata. Tempo max arrivo stimato: %.0f sec.",
                thread_id_for_log, emergency_id_str, max_time_to_arrive);
        log_message(LOG_EVENT_ASSIGNMENT, emergency_id_str, log_msg_buffer);

        // Allocare l'array di puntatori nell'emergenza
        current_emergency_node->data.rescuer_dt = (rescuer_digital_twin_t *)malloc(total_rescuers_needed * sizeof(rescuer_digital_twin_t));
        if (!current_emergency_node->data.rescuer_dt) {
             sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Fallimento MALLOC finale per array soccorritori assegnati!", thread_id_for_log, emergency_id_str);
             log_message(LOG_EVENT_GENERAL_ERROR, emergency_id_str, log_msg_buffer);
             free(selected_rescuers); // Libera l'array temporaneo di puntatori
             current_emergency_node->data.status = CANCELED;
             log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a CANCELED (malloc fallito).");
             free(current_emergency_node);
             current_emergency_node = NULL;
             continue;
        }
        
        // --- Blocco Critico: Aggiornamento stato soccorritori GLOBALI e COPIA dati nell'emergenza ---
        mtx_lock(&digital_twins_array_mutex);
        bool assignment_conflict = false;
        for (int i = 0; i < total_rescuers_needed; ++i) {
            rescuer_digital_twin_t *dt_global_to_assign = selected_rescuers[i]; // Questo è il puntatore al DT globale

            if (dt_global_to_assign->status != IDLE) {
                // Conflitto! Un altro thread ha preso questo soccorritore nel frattempo
                sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - CONFLITTO! Soccorritore ID %d non più IDLE durante assegnazione finale.",
                        thread_id_for_log, emergency_id_str, dt_global_to_assign->id);
                log_message(LOG_EVENT_ASSIGNMENT, emergency_id_str, log_msg_buffer);
                assignment_conflict = true;
                // Rollback dello stato per quelli già cambiati nel global_digital_twins_array
                for (int k = 0; k < i; ++k) {
                     // selected_rescuers[k] punta al DT globale il cui stato era stato cambiato
                     selected_rescuers[k]->status = IDLE;
                }
                break; // Esci dal loop di assegnazione
            }

            // 1. Aggiorna lo stato del soccorritore GLOBALE
            dt_global_to_assign->status = EN_ROUTE_TO_SCENE;

            // 2. MODIFICA 2: Copia la STRUTTURA del soccorritore globale
            //    nella i-esima posizione dell'array dell'emergenza.
            //    Nota: dt_global_to_assign->status è già EN_ROUTE_TO_SCENE qui.
            current_emergency_node->data.rescuer_dt[i] = *dt_global_to_assign; // COPIA LA STRUTTURA

            char rescuer_id_str[20];
            snprintf(rescuer_id_str, sizeof(rescuer_id_str), "Rescuer_%d", dt_global_to_assign->id);
            sprintf(log_msg_buffer, "Soccorritore ID %d ('%s') assegnato a Emergenza %s. Stato GLOBALE -> EN_ROUTE_TO_SCENE. Dati copiati in emergenza.",
                    dt_global_to_assign->id, dt_global_to_assign->rescuer->rescuer_type_name, emergency_id_str);
            log_message(LOG_EVENT_RESCUER_STATUS, rescuer_id_str, log_msg_buffer);
        }
        mtx_unlock(&digital_twins_array_mutex);
        // --- Fine Blocco Critico ---

        free(selected_rescuers); // Non serve più l'array temporaneo

        if (assignment_conflict) {
            // L'assegnazione è fallita all'ultimo secondo
            // L'array nell'emergenza contiene copie parziali (o nessuna se conflitto al primo), ma va liberato.
            free(current_emergency_node->data.rescuer_dt);
            current_emergency_node->data.rescuer_dt = NULL;
            current_emergency_node->data.status = TIMEOUT;
            log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a TIMEOUT (conflitto assegnazione).");
            free(current_emergency_node);
            current_emergency_node = NULL;
            continue;
        }
        

        // Assegnazione completata con successo!
        current_emergency_node->data.resquer_cont = total_rescuers_needed;
        current_emergency_node->data.status = ASSIGNED;
        log_message(LOG_EVENT_EMERGENCY_STATUS, emergency_id_str, "Stato emergenza impostato a ASSIGNED.");

        // --- FINE DELLA PARTE TODO SPECIFICA PER L'ASSEGNAZIONE ---

        // Da qui in poi inizia la gestione del ciclo di vita dell'emergenza
        // (Simulazione viaggio, arrivo, lavoro, ritorno)
        // Questo può essere implementato come passi successivi.

        // Placeholder per ora:
        sprintf(log_msg_buffer, "Thread #%d: Emergenza %s - Gestione ciclo vita (TODO)...", thread_id_for_log, emergency_id_str);
        log_message(LOG_EVENT_GENERAL_INFO, emergency_id_str, log_msg_buffer);  

        // Esempio di simulazione (da implementare correttamente)
        // sleep(max_time_to_arrive);
        // // Lock, change status dt -> ON_SCENE, emergency -> IN_PROGRESS, unlock, log...
        // sleep(work_time);
        // // Lock, change status dt -> RETURNING, emergency -> COMPLETED, unlock, log...
        // sleep(return_time);
        // // Lock, change status dt -> IDLE, unlock, log...


        // Alla fine del ciclo di vita (o se l'emergenza viene cancellata/timeout dopo l'assegnazione):
        if (current_emergency_node->data.rescuer_dt != NULL) {
             free(current_emergency_node->data.rescuer_dt); // Libera l'array di STRUTTURE COPIATE
             current_emergency_node->data.rescuer_dt = NULL;
        }
        free(current_emergency_node); // Libera il nodo della lista
        current_emergency_node = NULL;



    } // fine while(!shutdown_flag)

    sprintf(log_msg_buffer, "Thread Gestore Emergenze #%ld in terminazione.", thread_id_for_log);
    log_message(LOG_EVENT_GENERAL_INFO, "HANDLER_THREAD", log_msg_buffer);

    free(thread_args);
    return 0;
}











int main(int argc, char *argv[]) {
    char log_msg_buffer[LINE_LENGTH + 250]; // Buffer generico per log nel main

    // 1. Inizializzazione del Logger
    if (!init_logger("emergency_system.log")) {
        // Se init_logger fallisce, stampa su stderr (init_logger dovrebbe già farlo)
        // e non possiamo usare log_message.
        return EXIT_FAILURE;
    }
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Sistema di Gestione Emergenze Avviato.");

    // Flag per tracciare lo stato dell'inizializzazione per la pulizia
    bool config_fully_loaded = false;
    bool digital_twins_initialized = false;
    bool message_queue_open = false; 
    bool waiting_queue_sync_primitives_initialized = false; // Rinominiamo il flag esistente per chiarezza
    bool digital_twins_mutex_initialized = false;          // NUOVO FLAG



    // Inizializza i membri puntatore di global_system_config
    global_system_config.rescuers_type_array = NULL;
    global_system_config.instances_per_rescuer_type = NULL;
    global_system_config.emergency_types_array = NULL;
    global_system_config.rescuer_type_num = 0;
    global_system_config.emergency_type_num = 0;
    global_system_config.total_digital_twin_da_creare = 0;

    
    // 2. Parsing della Configurazione
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione ambiente (env.conf)...");
    if (!parse_environment("env.conf", &global_system_config.config_env)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di env.conf. Uscita.");
        cleanup_before_exit(false, false, false, false, false);
        return EXIT_FAILURE;
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione soccorritori (rescuers.conf)...");
    if (!parse_rescuers("rescuers.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di rescuers.conf. Uscita.");
        cleanup_before_exit(true, false, false, false, false); // true perché env.conf potrebbe essere parzialmente ok,
                                          // free_system_config gestirà i puntatori nulli.
        return EXIT_FAILURE;
    }  

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione tipi di emergenze (emergency_types.conf)...");
    if (!parse_emergency_types("emergency_types.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di emergency_types.conf. Uscita.");
        cleanup_before_exit(true, false, false, false, false);
        return EXIT_FAILURE;
    }
    config_fully_loaded = true;
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Tutta la configurazione è stata letta con successo.");

    // 3. Validazione Configurazione (Semplice esempio: ci sono soccorritori da creare?)
    if (global_system_config.total_digital_twin_da_creare <= 0 && global_system_config.rescuer_type_num > 0) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Configurazione indica tipi di soccorritori ma nessun gemello digitale da creare complessivamente.");
        // Potrebbe essere un errore fatale a seconda dei requisiti
    }
    if (global_system_config.total_digital_twin_da_creare == 0 && global_system_config.emergency_type_num > 0) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Tipi di emergenze definiti, ma nessun soccorritore disponibile. Uscita.");
        cleanup_before_exit(config_fully_loaded, false, false, false, false);
        return EXIT_FAILURE;
    }
    // Altre validazioni (es. coordinate base dentro la griglia) possono essere aggiunte qui.
    // Esempio:
    for (int i = 0; i < global_system_config.rescuer_type_num; ++i) {
        rescuer_type_t *rt = &global_system_config.rescuers_type_array[i];
        
        if (rt->x < 0 || rt->x >= global_system_config.config_env.grid_width ||
            rt->y < 0 || rt->y >= global_system_config.config_env.grid_height) {
            sprintf(log_msg_buffer, "Coordinate base (%d,%d) per soccorritore '%s' fuori dalla griglia (%dx%d). Uscita.",
                    rt->x, rt->y, rt->rescuer_type_name,
                    global_system_config.config_env.grid_width, global_system_config.config_env.grid_height);
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, false, false, false, false);
            return EXIT_FAILURE;
        }
    }


    // 4. Inizializzazione Gemelli Digitali
    if (global_system_config.total_digital_twin_da_creare > 0) {
        global_digital_twins_array = (rescuer_digital_twin_t *)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if (!global_digital_twins_array) {
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: %s", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, false, false, false, false);
            return EXIT_FAILURE;
        }
        digital_twins_initialized = true;
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio creazione gemelli digitali...");

        int current_twin_index = 0;
        for (int i = 0; i < global_system_config.rescuer_type_num; ++i) {
            rescuer_type_t *current_rescuer_type = &global_system_config.rescuers_type_array[i];
            int num_instances_for_this_type = global_system_config.instances_per_rescuer_type[i];

            for (int j = 0; j < num_instances_for_this_type; ++j) {
                if (current_twin_index >= global_system_config.total_digital_twin_da_creare) {
                    // Questo non dovrebbe accadere se total_digital_twin_da_creare è calcolato correttamente
                    log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore logico: Indice gemello digitale ha superato il totale allocato.");
                    cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, false, false);
                    return EXIT_FAILURE;
                }
                rescuer_digital_twin_t *twin = &global_digital_twins_array[current_twin_index];
                twin->id = current_twin_index + 1; // ID semplice basato sull'indice (inizia da 1)
                twin->rescuer = current_rescuer_type; // Puntatore al tipo
                twin->x = current_rescuer_type->x;    // Posizione base
                twin->y = current_rescuer_type->y;
                twin->status = IDLE;

                sprintf(log_msg_buffer, "Creato gemello digitale: ID=%d, Tipo='%s', Pos=(%d,%d), Stato=IDLE",
                        twin->id, twin->rescuer->rescuer_type_name, twin->x, twin->y);
                log_message(LOG_EVENT_RESCUER_STATUS, "SYSTEM", log_msg_buffer); // Usiamo SYSTEM come ID per la creazione
                current_twin_index++;
            }
        }
        total_digital_twins_created = current_twin_index;
        if (total_digital_twins_created != global_system_config.total_digital_twin_da_creare) {
             sprintf(log_msg_buffer, "Discordanza: Creati %d gemelli digitali, ma attesi %d.", total_digital_twins_created, global_system_config.total_digital_twin_da_creare);
             log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
             // Potrebbe essere un errore fatale
        }
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Creazione gemelli digitali completata.");
    } else {
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Nessun gemello digitale da creare secondo la configurazione.");
    }



    // 5. Inizializzazione Primitive di Sincronizzazione e gemelli digitali
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizializzazione primitive di sincronizzazione...");
    if (mtx_init(&waiting_emergencies_mutex, mtx_plain) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento inizializzazione mutex per coda emergenze. Uscita.");
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, false, false);
        return EXIT_FAILURE;
    }

    if (cnd_init(&new_emergency_condition) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento inizializzazione variabile di condizione. Uscita.");
        mtx_destroy(&waiting_emergencies_mutex); // Distruggi il mutex già inizializzato
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, false, false);
        return EXIT_FAILURE;
    }
    waiting_queue_sync_primitives_initialized = true; // Imposta il flag rinominato
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Primitive di sincronizzazione per coda attesa inizializzate.");


    //gemelli digitali
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizializzazione mutex per array gemelli digitali...");
    if (mtx_init(&digital_twins_array_mutex, mtx_plain) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento inizializzazione mutex per array gemelli digitali. Uscita.");
        // Chiamata a cleanup più completa
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, waiting_queue_sync_primitives_initialized, false);
        return EXIT_FAILURE;
    }

    digital_twins_mutex_initialized = true; // IMPOSTA IL NUOVO FLAG
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Mutex per array gemelli digitali inizializzato.");


    // 6. Inizializzazione Coda di Messaggi POSIX (Spostato dopo sync primitives per pulizia)
    struct mq_attr attributes;
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10; // Esempio, rendilo configurabile se necessario
    attributes.mq_msgsize = sizeof(emergency_request_t);
    attributes.mq_curmsgs = 0;

    sprintf(log_msg_buffer, "Apertura coda messaggi POSIX: /%s", global_system_config.config_env.queue_name); // Aggiungi '/' se non c'è già
    log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", log_msg_buffer);
    
    char mq_name_with_slash[LINE_LENGTH + 1];
    if (global_system_config.config_env.queue_name[0] != '/') {
        snprintf(mq_name_with_slash, sizeof(mq_name_with_slash), "/%s", global_system_config.config_env.queue_name);
    } else {
        strncpy(mq_name_with_slash, global_system_config.config_env.queue_name, sizeof(mq_name_with_slash) -1);
        mq_name_with_slash[sizeof(mq_name_with_slash)-1] = '\0';
    }

    
    // Prova a rimuovere la coda esistente prima, per evitare problemi se non è stata pulita correttamente
    // mq_unlink(mq_name_with_slash); // Attenzione: questo è aggressivo. Fallisce silenziosamente se non esiste.

    global_mq_descriptor = mq_open(mq_name_with_slash, O_CREAT | O_RDONLY | O_EXCL, 0660, &attributes);
    if (global_mq_descriptor == (mqd_t)-1) {    
        if (errno == EEXIST) { // La coda esiste già, proviamo ad aprirla senza O_CREAT | O_EXCL
            log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", "Coda messaggi già esistente, tentativo di riapertura...");
            global_mq_descriptor = mq_open(mq_name_with_slash, O_RDONLY);
        }
        if (global_mq_descriptor == (mqd_t)-1) { // Ancora errore
            sprintf(log_msg_buffer, "Fallimento mq_open per coda '%s': %s", mq_name_with_slash, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, waiting_queue_sync_primitives_initialized, digital_twins_mutex_initialized);
            return EXIT_FAILURE;
        }
    }
    message_queue_open = true;
    log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", "Coda messaggi aperta con successo.");

    // 7. Creazione del Thread Listener Coda Messaggi
    thrd_t mq_listener_thread_id;
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Creazione thread listener coda messaggi...");
    if (thrd_create(&mq_listener_thread_id, message_queue_listener_thread_func, NULL) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento creazione thread listener coda messaggi. Uscita.");
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, waiting_queue_sync_primitives_initialized, digital_twins_mutex_initialized);
        return EXIT_FAILURE;
    }
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Thread listener coda messaggi creato con successo.");




    // 8. Creazione dei Thread Gestori Emergenze

    #define NUM_HANDLER_THREADS 3 // Esempio: 3 thread gestori
    thrd_t handler_thread_ids[NUM_HANDLER_THREADS];
    thread_handler_args_t *handler_args_array[NUM_HANDLER_THREADS]; // Array di puntatori agli argomenti 

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Creazione thread gestori emergenze...");
    for (int i = 0; i < NUM_HANDLER_THREADS; ++i) {

        // Alloca memoria per gli argomenti di questo thread
        handler_args_array[i] = (thread_handler_args_t *)malloc(sizeof(thread_handler_args_t));
        if (!handler_args_array[i]) {
            char err_buf[150];
            sprintf(err_buf, "Fallimento allocazione memoria per argomenti thread gestore #%d. Uscita.", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", err_buf);
            
            // Pulizia parziale per i thread e argomenti già creati
            shutdown_flag = true;
            cnd_broadcast(&new_emergency_condition);
            
            int res_listener;
            if((thrd_join(mq_listener_thread_id, &res_listener)) != thrd_success){
                sprintf(log_msg_buffer, "Fallimento join thread listener durante cleanup errore: %d", res_listener);
                log_message(LOG_EVENT_GENERAL_ERROR, "MAIN_ERROR_CLEANUP", log_msg_buffer);
            }else{
                sprintf(log_msg_buffer, "Thread listener terminato (codice %d) durante cleanup errore.", res_listener);
                log_message(LOG_EVENT_GENERAL_INFO, "MAIN_ERROR_CLEANUP", log_msg_buffer);
            }


            /*
            for (int k_join = 0; k_join < i; ++k_join) {
                int res_handler;
                if (thrd_join(handler_thread_ids[k_join], &res_handler) != thrd_success) {
                    sprintf(log_msg_buffer, "Fallimento join thread gestore #%d durante cleanup errore: %d", k_join, res_handler);
                    log_message(LOG_EVENT_GENERAL_ERROR, "MAIN_ERROR_CLEANUP", log_msg_buffer);
                } else {
                    sprintf(log_msg_buffer, "Thread gestore #%d terminato (codice %d) durante cleanup errore.", k_join, res_handler);
                    log_message(LOG_EVENT_GENERAL_INFO, "MAIN_ERROR_CLEANUP", log_msg_buffer);
                }
            }
            */
            cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, waiting_queue_sync_primitives_initialized, digital_twins_mutex_initialized);
    
            return EXIT_FAILURE;
        }
        handler_args_array[i]->thread_id_for_log = i; // Assegna l'ID

        if (thrd_create(&handler_thread_ids[i], emergency_handler_thread_func, handler_args_array[i]) != thrd_success) {
            // Errore creazione thread: terminare gli altri già creati e pulire
            char err_buf[100];
            sprintf(err_buf, "Fallimento creazione thread gestore emergenze #%d. Uscita.", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", err_buf);
            
            shutdown_flag = true; // Segnala agli altri thread di terminare
            cnd_broadcast(&new_emergency_condition); // Sveglia i gestori in cnd_wait

            // Join dei thread creati finora (listener + gestori fino a i-1)
            thrd_join(mq_listener_thread_id, NULL); // Ignora risultato per semplicità qui
            for (int k = 0; k < i; ++k) {
                thrd_join(handler_thread_ids[k], NULL);
                free(handler_args_array[k]);    
            }
            free(handler_args_array[i]);
            cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, waiting_queue_sync_primitives_initialized, digital_twins_mutex_initialized);
            return EXIT_FAILURE;
        }
        sprintf(log_msg_buffer, "Thread gestore emergenze #%d creato.", i);
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", log_msg_buffer);
    }
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Tutti i thread gestori emergenze creati.");



    // Attendiamo che il thread listener termini
    int res_listener;
    if (thrd_join(mq_listener_thread_id, &res_listener) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento join thread listener coda messaggi.");
    } else {
        sprintf(log_msg_buffer, "Thread listener coda messaggi terminato con codice: %d.", res_listener);
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", log_msg_buffer);
    }



    // Segnalare ai thread di terminare e svegliarli
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Invio segnale di shutdown ai thread...");
    shutdown_flag = true;
    cnd_broadcast(&new_emergency_condition); // Sveglia tutti i gestori in attesa  

    // Per sbloccare il thread listener da mq_receive in modo pulito:
    // Una strategia è chiudere la coda dal main. Questo farà sì che mq_receive nel listener
    // restituisca un errore (probabilmente EBADF - Bad File Descriptor).
    // Il listener dovrebbe gestire questo errore e terminare se shutdown_flag è true.
    if (message_queue_open && global_mq_descriptor != (mqd_t)-1) {
        // log_message(LOG_EVENT_MESSAGE_QUEUE, "MAIN", "Chiusura forzata mq_descriptor per sbloccare listener.");
        // mq_close(global_mq_descriptor); // Listener gestirà errore e shutdown_flag
        // global_mq_descriptor = (mqd_t)-1; // Segna come chiuso per cleanup_before_exit
        // NOTA: chiudere qui potrebbe essere prematuro se cleanup_before_exit lo fa già.
        // Una soluzione migliore è usare segnali o mq_timedreceive nel listener.
        // Per ora, il listener si basa su un eventuale errore di mq_receive o sul
        // fatto che il prossimo messaggio lo sblocchi e veda shutdown_flag.
        // Un modo per forzare lo sblocco senza chiudere prematuramente è usare mq_notify
        // per farsi inviare un segnale, e il gestore di segnale imposta shutdown_flag
        // e il segnale interrompe mq_receive.
        // Alternativa semplice ma "brutta": inviare un messaggio fittizio alla coda.
    }

    // Attendiamo che i thread terminino
    int res_val;
    if (thrd_join(mq_listener_thread_id, &res_val) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento join thread listener coda messaggi.");
    } else {
        sprintf(log_msg_buffer, "Thread listener coda messaggi terminato con codice: %d.", res_val);
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", log_msg_buffer);
    }

    for (int i = 0; i < NUM_HANDLER_THREADS; ++i) {
        int res_val;
        if (thrd_join(handler_thread_ids[i], &res_val) != thrd_success) {
            sprintf(log_msg_buffer, "Fallimento join thread gestore emergenze #%d.", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
        } else {
            sprintf(log_msg_buffer, "Thread gestore emergenze #%d terminato con codice: %d.", i, res_val);
            log_message(LOG_EVENT_GENERAL_INFO, "MAIN", log_msg_buffer);
        }
        // La memoria per handler_args_array[i] ora viene liberata dal thread stesso alla sua uscita.
        // Se avessimo deciso che il main la libera, faremmo free(handler_args_array[i]); qui.
        // Dato che il thread la libera, non è necessario farlo qui.
        // Ma è buona norma impostare il puntatore a NULL dopo che il thread è terminato, se l'array persiste.
        // handler_args_array[i] = NULL; // Opzionale, l'array stesso uscirà di scope.
    }
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Tutti i thread di lavoro sono stati terminati.");


    // --- Pulizia Finale ---
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Programma in terminazione. Pulizia risorse...");
    cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, waiting_queue_sync_primitives_initialized, digital_twins_mutex_initialized);
    // close_logger() è già in cleanup_before_exit

    printf("Sistema terminato correttamente.\n");
    return EXIT_SUCCESS;



}