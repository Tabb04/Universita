#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h> // Per future implementazioni multithread
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

// Funzione di pulizia generale da chiamare prima di uscire in caso di errore parziale
void cleanup_before_exit(bool config_loaded, bool digital_twins_loaded /*, bool mq_loaded etc.*/) {
    if (digital_twins_loaded && global_digital_twins_array) {
        // Se i digital twin avessero memoria interna da liberare (non in questo caso), andrebbe fatto qui.
        free(global_digital_twins_array);
        global_digital_twins_array = NULL;
        log_message(LOG_EVENT_GENERAL_INFO, "CLEANUP", "Array gemelli digitali deallocato.");
    }
    if (config_loaded) {
        free_system_config(&global_system_config);
    }
    close_logger();
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
    // bool message_queue_open = false; // Per il futuro

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
        cleanup_before_exit(false, false);
        return EXIT_FAILURE;
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione soccorritori (rescuers.conf)...");
    if (!parse_rescuers("rescuers.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di rescuers.conf. Uscita.");
        cleanup_before_exit(true, false); // true perché env.conf potrebbe essere parzialmente ok,
                                          // free_system_config gestirà i puntatori nulli.
        return EXIT_FAILURE;
    }

    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizio parsing configurazione tipi di emergenze (emergency_types.conf)...");
    if (!parse_emergency_types("emergency_types.conf", &global_system_config)) {
        log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", "Errore fatale durante il parsing di emergency_types.conf. Uscita.");
        cleanup_before_exit(true, false);
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
        cleanup_before_exit(config_fully_loaded, digital_twins_initialized);
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
            cleanup_before_exit(config_fully_loaded, digital_twins_initialized);
            return EXIT_FAILURE;
        }
    }


    // 4. Inizializzazione Gemelli Digitali
    if (global_system_config.total_digital_twin_da_creare > 0) {
        global_digital_twins_array = (rescuer_digital_twin_t *)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if (!global_digital_twins_array) {
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: %s", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, false);
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
                    cleanup_before_exit(config_fully_loaded, digital_twins_initialized);
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

    // 5. Inizializzazione Coda di Messaggi POSIX (Esempio base)
    // mqd_t mq_descriptor;
    // struct mq_attr attributes;
    // attributes.mq_flags = 0; // Blocking (default)
    // attributes.mq_maxmsg = 10; // Max 10 messaggi in coda (configurabile)
    // attributes.mq_msgsize = sizeof(emergency_request_t); // Dimensione di ogni messaggio
    // attributes.mq_curmsgs = 0; // Ignorato da mq_open

    // sprintf(log_msg_buffer, "Apertura coda messaggi POSIX: %s", global_system_config.config_env.queue_name);
    // log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", log_msg_buffer);

    // // O_CREAT: crea se non esiste. O_RDONLY: Solo lettura per il server.
    // // Permessi: 0660 (lettura/scrittura per utente e gruppo)
    // mq_descriptor = mq_open(global_system_config.config_env.queue_name, O_CREAT | O_RDONLY, 0660, &attributes);
    // if (mq_descriptor == (mqd_t)-1) {
    //     sprintf(log_msg_buffer, "Fallimento mq_open per coda '%s': %s", global_system_config.config_env.queue_name, strerror(errno));
    //     log_message(LOG_EVENT_GENERAL_ERROR, "MAIN", log_msg_buffer);
    //     // USA LA TUA MACRO QUI SE PREFERISCI, ma la macro dovrebbe gestire il return e cleanup
    //     cleanup_before_exit(config_fully_loaded, digital_twins_initialized);
    //     return EXIT_FAILURE;
    // }
    // message_queue_open = true;
    // log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", "Coda messaggi aperta con successo.");


    // --- QUI INIZIERANNO I THREAD E IL CICLO PRINCIPALE ---
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Inizializzazione base completata. Il sistema entrerebbe nel ciclo operativo.");
    // Per ora, simuliamo una terminazione pulita dopo l'inizializzazione.
    // In un sistema reale, qui si avviano i thread e si attende un segnale di shutdown.

    // Esempio di come si potrebbe usare un'emergenza e un soccorritore (molto semplificato)
    // if (total_digital_twins_created > 0 && global_system_config.emergency_type_num > 0) {
    //     emergency_t test_emergency;
    //     test_emergency.type = &global_system_config.emergency_types_array[0]; // Prendi il primo tipo di emergenza
    //     test_emergency.x = 10;
    //     test_emergency.y = 10;
    //     test_emergency.time = time(NULL);
    //     test_emergency.status = WAITING;
    //     test_emergency.resquer_cont = 1; // Supponiamo serva 1 soccorritore per questo test
    //     test_emergency.rescuer_dt = malloc(1 * sizeof(rescuer_digital_twin_t));
    //     if (test_emergency.rescuer_dt) {
    //         // Copia il primo soccorritore disponibile (questo è solo un test grezzo)
    //         memcpy(test_emergency.rescuer_dt, &global_digital_twins_array[0], sizeof(rescuer_digital_twin_t));
    //         global_digital_twins_array[0].status = EN_ROUTE_TO_SCENE; // Aggiorna lo stato del gemello globale
    //
    //         sprintf(log_msg_buffer, "Test: Emergenza '%s' assegnata a Soccorritore ID %d",
    //                 test_emergency.type->emergency_desc, test_emergency.rescuer_dt[0].id);
    //         log_message(LOG_EVENT_ASSIGNMENT, "TEST_EMERG_1", log_msg_buffer);
    //
    //         free(test_emergency.rescuer_dt); // Pulizia del test
    //     }
    // }


    // --- Pulizia Finale ---
    log_message(LOG_EVENT_GENERAL_INFO, "MAIN", "Programma in terminazione. Pulizia risorse...");
    
    // if (message_queue_open) {
    //     mq_close(mq_descriptor);
    //     // Considera mq_unlink se questo server è l'unico responsabile della coda
    //     // if (mq_unlink(global_system_config.config_env.queue_name) == -1) {
    //     //    sprintf(log_msg_buffer, "Attenzione: Fallimento mq_unlink per coda '%s': %s", global_system_config.config_env.queue_name, strerror(errno));
    //     //    log_message(LOG_EVENT_GENERAL_WARNING, "MAIN", log_msg_buffer);
    //     // } else {
    //     //    log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", "Coda messaggi rimossa con successo.");
    //     // }
    //     log_message(LOG_EVENT_MESSAGE_QUEUE, "SYSTEM", "Coda messaggi chiusa.");
    // }

    cleanup_before_exit(config_fully_loaded, digital_twins_initialized /*, message_queue_open */);
    // close_logger() è già in cleanup_before_exit

    return EXIT_SUCCESS;
}