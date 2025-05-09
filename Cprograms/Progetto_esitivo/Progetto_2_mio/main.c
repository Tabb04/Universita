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
system_config_t global_system_config;
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


    global_system_config.rescuers_type_array = NULL;
    global_system_config.instances_per_rescuer_type = NULL;
    global_system_config.emergency_types_array = NULL;
    global_system_config.rescuer_type_num = 0;
    global_system_config.emergency_type_num = 0;
    global_system_config.total_digital_twin_da_creare = 0;

    //inizio con tutti i parsing

    //parsing configurazione
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizio configurazione ambiente da env.conf");
    if(!parse_environment("env.conf", &global_system_config.config_env)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di env.conf");
        pulizia_e_uscita(false, false);     //nessuno dei due caricato in teoria
        return EXIT_FAILURE;
    }

    //parsing soccorritori
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Inizio configurazione soccorritori da rescuers.env");
    if(!parse_rescuers("rescuers.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di rescuers.conf");
        pulizia_e_uscita(true, false);  //true perchè env.conf potrebbe essere potenzialmente ok
        return EXIT_FAILURE;
    }

    //parsing emergenze
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Inizio configurazione emergenze da emergency_types.env");
    if(!parse_rescuers("emergency_types.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di emergency_types.conf");
        pulizia_e_uscita(true, false);
        return EXIT_FAILURE;
    }

    //andato tutto bene
    config_fully_loaded = true;
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Configurazione avvenuta con successo");


    //controllo configurazione basic
    
    //no gemelli digitali
    if((global_system_config.total_digital_twin_da_creare == 0) && (global_system_config.rescuer_type_num > 0)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Presenti tipi di soccorritori ma nessun gemello digitale");
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati);
        return EXIT_FAILURE;
    }

    //coordinate
    for(int i = 0; i<global_system_config.rescuer_type_num; i++){
        rescuer_type_t *resc = &global_system_config.rescuers_type_array[i];

        if((resc->x) < 0 || (resc->y) < 0 || (resc->x >= global_system_config.config_env.grid_width) || (resc->y >= global_system_config.config_env.grid_height)){
            sprintf(log_msg_buffer, "Coordinate base del soccorritore '%s' fuori dalla griglia", resc->rescuer_type_name);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati);
            return EXIT_FAILURE;
        }
    }


    //ora inizializzo i gemelli digitali
    if(global_system_config.total_digital_twin_da_creare > 0){
        global_digital_twins_array = (rescuer_digital_twin_t*)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if(!global_digital_twins_array){
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: '%s'", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, false);
            return EXIT_FAILURE;
        }
        
        sprintf(log_msg_buffer, "Allocato spazio per %d gemelli digitali: '%s'", global_system_config.total_digital_twin_da_creare, strerror(errno));
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);

        int indice_gemello = 0;
        for(int i = 0; i<global_system_config.emergency_type_num; i++){
            rescuer_type_t* tipo_corrente = &global_system_config.emergency_types_array[i];
            int istanze_da_fare = global_system_config.instances_per_rescuer_type[i];   //array che mi ero fatto per quanti ad ogn tipo
            //sarebbe stato meglio mettelo dentro il tipo stesso ma prof non mi risponde

            for(int j = 0; j<istanze_da_fare; j++){
                rescuer_digital_twin_t* gemello = &global_digital_twins_array[indice_gemello];
                gemello->id = indice_gemello+1;  //faccio l'id in base all'indice
                gemello->rescuer = tipo_corrente;
                gemello->x = tipo_corrente->x;   //parte dalla base
                gemello->y = tipo_corrente->y;
                gemello->status = IDLE;  //guarda elenco stato pdf

                sprintf(log_msg_buffer, "Gemello digitale creato, ID=%d, Tipo=%s", gemello->id, gemello->rescuer->rescuer_type_name);
                log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
                indice_gemello++;
            }
        }

        //indice è quanti ne ho creati
        total_digital_twins_creati = indice_gemello;
        if(total_digital_twins_creati != global_system_config.total_digital_twin_da_creare){
            sprintf(log_msg_buffer, "Discrepanza tra gemelli da creare %d e gemelli creati %d", global_system_config.total_digital_twin_da_creare, total_digital_twins_creati);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            cleanup_before_exit(config_fully_loaded, digital_twins_inizializzati);
            return EXIT_FAILURE;
        }

        sprintf(log_msg_buffer, "Creati correttamente %d gemelli", global_system_config.total_digital_twin_da_creare);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
        
    }else{
        log_message(LOG_EVENT_GENERAL_INFO, "Main", "Nessun gemello creato come descritto da file");
    }

    //inizializzazione coda


    return EXIT_SUCCESS;


}

