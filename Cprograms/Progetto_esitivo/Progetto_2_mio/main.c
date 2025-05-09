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

#define NOME_LOGGER "emergency_system.log"


//variabili globali
system_config_t* global_system_config;
rescuer_digital_twin_t* global_digital_twins_array = NULL;
int total_digital_twins_creati = 0;


void pulizia_e_uscita(bool config_loaded, bool digital_twins_loaded){
    if(digital_twins_loaded && global_digital_twins_array){
        free(global_digital_twins_array);
        global_digital_twins_array = NULL;
        log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Deallocato array gemelli digitali");
    }
    if(config_loaded){
        free_system_config(&global_system_config);
    }
    close_logger();
}

int main(int argc, char* argv[]){
    char log_msg_buffer[LINE_LENGTH + 250];

    //inizializzo logger
    if(!init_logger(NOME_LOGGER)){
        return EXIT_FAILURE;    //non posso scrivere su file, duh
    }

    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Sistema di Emergenze avviato");

    //flag per tracciare lo stato di inizializzaione per pulizia (a false)
    bool config_fully_loaded = false;
    bool digital_twins_inizializzati = false;   
    bool message_queue_open = false;


    global_system_config->rescuers_type_array = NULL;
    global_system_config->instances_per_rescuer_type = NULL;
    global_system_config->emergency_types_array = NULL;
    global_system_config->rescuer_type_num = 0;
    global_system_config->emergency_type_num = 0;
    global_system_config->total_digital_twin_da_creare = 0;

    //inizio con tutti i parsing

    //parsing configurazione
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizio configurazione ambiente da env.conf");
    if(!parse_environment("env.conf", &global_system_config->config_env)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di env.conf");
        pulizia_e_uscita
    }


}

