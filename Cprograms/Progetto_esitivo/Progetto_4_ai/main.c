#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h> // Per future implementazioni multithread
#include <mqueue.h>  // Per code di messaggi POSIX
#include <fcntl.h>   // Per O_flags in mq_open
#include <sys/stat.h> // Per mode in mq_open
#include <errno.h>

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

emergency_node_t *waiting_emergencies_list_head = NULL; // Testa della lista
emergency_node_t *waiting_emergencies_list_tail = NULL; // Coda della lista per aggiunte O(1)

mtx_t waiting_emergencies_mutex; // Mutex per proteggere la lista
cnd_t new_emergency_condition;   // Variabile di condizione per segnalare nuove emergenze


// --- Variabile per la terminazione controllata ---
volatile bool shutdown_flag = false; // Impostata a true per terminare i thread

// --- Descrittore della coda di messaggi POSIX ---
mqd_t global_mq_descriptor = (mqd_t)-1; // Inizializza a un valore non valido



// Funzione di pulizia generale da chiamare prima di uscire in caso di errore parziale
void cleanup_before_exit(bool config_loaded, bool digital_twins_loaded, bool mq_loaded, bool sync_primit) {
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

    if (sync_primit) {
        mtx_destroy(&waiting_emergencies_mutex);
        cnd_destroy(&new_emergency_condition);
        log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Primitive di sincronizzazione distrutte.");
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
    bool message_queue_open = false; // AGGIUNTO
    bool sync_primitives_initialized = false; //AGGIUNTO


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
        cleanup_before_exit(false, false, false, false);
        return EXIT_FAILURE;
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione soccorritori (rescuers.conf)...");
    if (!parse_rescuers("rescuers.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di rescuers.conf. Uscita.");
        cleanup_before_exit(true, false, false, false); // true perché env.conf potrebbe essere parzialmente ok,
                                          // free_system_config gestirà i puntatori nulli.
        return EXIT_FAILURE;
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione tipi di emergenze (emergency_types.conf)...");
    if (!parse_emergency_types("emergency_types.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di emergency_types.conf. Uscita.");
        cleanup_before_exit(true, false, false, false);
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
        cleanup_before_exit(config_fully_loaded, false, false, false);
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
            cleanup_before_exit(config_fully_loaded, false, false, false);
            return EXIT_FAILURE;
        }
    }


    // 4. Inizializzazione Gemelli Digitali
    if (global_system_config.total_digital_twin_da_creare > 0) {
        global_digital_twins_array = (rescuer_digital_twin_t *)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if (!global_digital_twins_array) {
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: %s", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, false, false, false);
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
                    cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, false);
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



    // 5. Inizializzazione Primitive di Sincronizzazione
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizializzazione primitive di sincronizzazione...");
    if (mtx_init(&waiting_emergencies_mutex, mtx_plain) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento inizializzazione mutex per coda emergenze. Uscita.");
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, false);
        return EXIT_FAILURE;
    }

    if (cnd_init(&new_emergency_condition) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento inizializzazione variabile di condizione. Uscita.");
        mtx_destroy(&waiting_emergencies_mutex); // Distruggi il mutex già inizializzato
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, false);
        return EXIT_FAILURE;
    }
    sync_primitives_initialized = true;
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Primitive di sincronizzazione inizializzate.");



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
            cleanup_before_exit(config_fully_loaded, digital_twins_initialized, false, sync_primitives_initialized);
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
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, sync_primitives_initialized);
        return EXIT_FAILURE;
    }
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Thread listener coda messaggi creato con successo.");

     // --- QUI INIZIERANNO I THREAD GESTORI EMERGENZE E IL CICLO PRINCIPALE DI SUPERVISIONE ---
     log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizializzazione completata. Il sistema è operativo.");
    
     // Esempio di ciclo di attesa per la terminazione (es. tramite CTRL+C o comando)
     // Per ora, facciamo un semplice join del thread listener.
     // In un sistema reale, avresti un loop qui che potrebbe fare altro o attendere un segnale.
     // Per terminare, imposta shutdown_flag = true; e possibilmente invia un messaggio fittizio
     // alla coda o usa mq_notify con un segnale per sbloccare mq_receive se è bloccato.
     // Un modo semplice per sbloccare mq_receive per il check di shutdown_flag:
     // Dopo aver impostato shutdown_flag = true, se il thread è bloccato su mq_receive,
     // potresti chiudere il descrittore della coda da un altro thread (main) o inviare un segnale
     // al thread listener. Chiudere mq_descriptor da main farà fallire mq_receive nel listener.
 
     // Semplice attesa. In pratica, qui ci sarebbe la logica di gestione dei segnali di shutdown.
     // Per un test, potresti fare:
     // printf("Premi INVIO per terminare il sistema...\n");
     // getchar(); 
     // shutdown_flag = true;
     // A questo punto, per sbloccare il thread listener se è in mq_receive:
     // 1. Potresti chiudere la coda dal main: mq_close(global_mq_descriptor); global_mq_descriptor = (mqd_t)-1;
     //    Questo farà fallire mq_receive nel thread listener con EBADF.
     // 2. Oppure usare segnali e pthread_kill (o l'equivalente C11 se c'è per i segnali ai thread)
     //    per interrompere la chiamata bloccante.
    
    // Attendiamo che il thread listener termini
    int res_listener;
    if (thrd_join(mq_listener_thread_id, &res_listener) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Fallimento join thread listener coda messaggi.");
    } else {
        sprintf(log_msg_buffer, "Thread listener coda messaggi terminato con codice: %d.", res_listener);
        log_message(LOG_EVENT_GENERAL_INFO, "MAIN", log_msg_buffer);
    }

    // --- Pulizia Finale ---
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Programma in terminazione. Pulizia risorse...");
    cleanup_before_exit(config_fully_loaded, digital_twins_initialized, message_queue_open, sync_primitives_initialized);
    // close_logger() è già in cleanup_before_exit

    printf("Sistema terminato correttamente.\n");
    return EXIT_SUCCESS;



}