#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "Syscalls_3_progetto.h"
#include "parser.h"
#include "data_types.h"
#include "config1.h"
#include "logger.h" 



static void rimuovi_spazi(char* str){
    if (!str){
        
        return;
    }

    
    while(isspace((unsigned char)*str)){    
        str++;
    }

    if(*str == 0){  
        return;     
    }

    
    char *fine;
    fine = str+strlen(str)-1;   
    while(isspace((unsigned char)*fine) && (fine > str)){
        fine--; 
    }
    fine[1] = '\0'; 
}

bool parse_rescuers(const char* nome_file, system_config_t* config){
    char log_msg_buffer[LINE_LENGTH + 200];

    if(!nome_file || !config){
        sprintf(log_msg_buffer, "Parsing fallito per parametri nulli");
        log_message(LOG_EVENT_FILE_PARSING, "parse_rescuers", log_msg_buffer);
        return false;
    }

    sprintf(log_msg_buffer, "Tentativo di apertura file '%s'", nome_file);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    FILE* file;
    sprintf(log_msg_buffer, "Errore fopen apertura file '%s': %s", nome_file, strerror(errno));
    LOG_SNCALL(file, fopen(nome_file, "r"), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);


    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    

    char riga[LINE_LENGTH];     
    int riga_num_pass1 = 0;     
    int type_count = 0;         

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Primo passaggio: Conto pontenziali tipi di soccorritori");
    while(fgets(riga, sizeof(riga), file)){
        riga_num_pass1++;
        
        char riga_copia[LINE_LENGTH];   
        strncpy(riga_copia, riga, LINE_LENGTH - 1);
        riga_copia[LINE_LENGTH - 1] = '\0';

        riga_copia[strcspn(riga_copia, "\n")] = '\0'; 
        rimuovi_spazi(riga_copia);

        if(strlen(riga_copia) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        char temp_nome[RESCUER_NAME_LENGTH];
        int temp_num;
        int temp_speed;
        int temp_x;
        int temp_y;

        if(sscanf(riga_copia, " [%[^]]] [%d] [%d] [%d;%d]", temp_nome, &temp_num, &temp_speed, &temp_x, &temp_y) == 5){
            
            
            type_count++;
        }
    }

    sprintf(log_msg_buffer, "Primo passsaggio: Completato, trovati potenziali %d tipi di soccorritori", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    
    if(type_count == 0){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Nessun tipo di soccorritore trovato");
        config->rescuer_type_num = 0;
        config->rescuers_type_array = NULL;
        config->total_digital_twin_da_creare = 0;
        config->instances_per_rescuer_type = NULL;
        if(fclose(file) == EOF){
            sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
            file = NULL;
        }
        return true;    
    }

    
    config->rescuers_type_array = (rescuer_type_t *)malloc(type_count * sizeof(rescuer_type_t));
    
    if(!config->rescuers_type_array){
        
        sprintf(log_msg_buffer, "Errore su malloc su allocazione per %d tipi di soccorritore: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        if(fclose(file) == EOF){
            sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
            file = NULL;
        }
        return false;
    }
    sprintf(log_msg_buffer, "Allocato spazio per %d soccorritori", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);


    config->instances_per_rescuer_type = malloc(type_count * sizeof(int));
    if (!config->instances_per_rescuer_type) {
        sprintf(log_msg_buffer, "Errore su malloc per instances_per_rescuer_type per %d interi: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        free(config->rescuers_type_array); 
        config->rescuers_type_array = NULL;
        if(fclose(file) == EOF){
            sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
            file = NULL;
        }
        return false;
    }
    sprintf(log_msg_buffer, "Allocato instances_per_rescuer_type per %d interi.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);


    
    config->total_digital_twin_da_creare = 0;
    config->rescuer_type_num = 0;

    
    rewind(file);
    int riga_num_pass2 = 0;
    int indice_tipo = 0;
    


    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Secondo passsaggio: Estraggo dati");

    while(fgets(riga, sizeof(riga), file) && indice_tipo < type_count){
        riga_num_pass2++;
        
        
        char riga_copia1[LINE_LENGTH];  
        
        strncpy(riga_copia1, riga, LINE_LENGTH -1);
        riga_copia1[LINE_LENGTH - 1] = '\0';
        riga_copia1[strcspn(riga_copia1, "\n")] = '\0'; 
        rimuovi_spazi(riga_copia1);

        if(strlen(riga) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass2);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        char nome[RESCUER_NAME_LENGTH];
        int num;    
        int speed;
        int base_x;
        int base_y;

        int found = sscanf(riga, " [%[^]]] [%d] [%d] [%d;%d]", nome, &num, &speed, &base_x, &base_y);
        
        if(found != 5){
            
            sprintf(log_msg_buffer, "Errore riga %d, riga malformata -> '%s'", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        if(num <= 0 || speed <= 0 || base_x <= 0 || base_y <=0 || strlen(nome)==0 || strlen(nome) >= RESCUER_NAME_LENGTH){
            sprintf(log_msg_buffer, "Errore riga %d, uno o più valori non validi -> (num=%d, speed=%d, base_x=%d, base_y=%d, lunghezza_nome=%zu), '%s'", riga_num_pass2, num, speed, base_x, base_y, strlen(nome), riga_copia1);
            
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        
        rescuer_type_t *current = &config->rescuers_type_array[indice_tipo];
        

        current->rescuer_type_name = (char*)malloc(strlen(nome) + 1);
        if(current->rescuer_type_name != NULL){
            
            strcpy(current->rescuer_type_name, nome);
        }else{
            sprintf(log_msg_buffer, "Errore riga %d, fallita allocazione memoria -> nome soccorritore=%s, '%s'", riga_num_pass2, nome, strerror(errno));
            
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

            for(int i = 0; i<indice_tipo; i++){
                if(config->rescuers_type_array[i].rescuer_type_name){
                    free(config->rescuers_type_array[i].rescuer_type_name);
                }
            }

            free(config->rescuers_type_array); 
            config->rescuers_type_array = NULL;
            config->rescuer_type_num = 0;
            if(fclose(file) == EOF){
                sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                file = NULL;
            }
            return false;
        }
        
        current->speed = speed;
        current->x = base_x;
        current->y = base_y;
        config->total_digital_twin_da_creare += num;
        config->instances_per_rescuer_type[indice_tipo] = num;

        sprintf(log_msg_buffer, "Estratto soccorritore da riga %d: Nome=%s, Velocità=%d, Base=(%d,%d). Numero unità=%d", riga_num_pass2, current->rescuer_type_name, current->speed, current->x, current->y, num);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

        indice_tipo++;
    }

    config->rescuer_type_num = indice_tipo;   
    if(fclose(file) == EOF){
        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
        file = NULL;
    }

    
    if(config->rescuer_type_num == 0 && type_count > 0){
        sprintf(log_msg_buffer, "Trovate %d righe potenziali ma nessuna convalidata", type_count);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        free(config->rescuers_type_array); 
        config->rescuers_type_array = NULL;
        return false;   
    }

    
    sprintf(log_msg_buffer, "Parsing dei soccorritori completato. Letti %d tipi di soccorritori, vanno creati totali %d gemelli virtuali", config->rescuer_type_num, config->total_digital_twin_da_creare);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    return true;
}





void free_system_config(system_config_t* config){
    char log_msg_buffer[200];

    if(!config){
        return; 
    }

    log_message(LOG_EVENT_GENERAL_INFO, "System_config", "Inizio deallocazione system_config");


    
    if(config->rescuers_type_array){
        for(int i = 0; i<config->rescuer_type_num; i++){
            if(config->rescuers_type_array[i].rescuer_type_name){
                free(config->rescuers_type_array[i].rescuer_type_name);
                config->rescuers_type_array[i].rescuer_type_name = NULL;
            }
        }

        free(config->rescuers_type_array);
        config->rescuers_type_array = NULL;

        sprintf(log_msg_buffer, "Deallogati %d tipi di soccorritori", config->rescuer_type_num);
        log_message(LOG_EVENT_GENERAL_INFO, "System_config", log_msg_buffer);
        config->rescuer_type_num = 0;
    }

    
    if(config->instances_per_rescuer_type) {
        free(config->instances_per_rescuer_type);
        config->instances_per_rescuer_type = NULL;
        sprintf(log_msg_buffer, "Deallocato instances_per_rescuer_type.");
        log_message(LOG_EVENT_GENERAL_INFO, "System_config", log_msg_buffer);
    }


    
    if(config->emergency_types_array){
        for(int i = 0; i<config->emergency_type_num; i++){
            if(config->emergency_types_array[i].emergency_desc){
                free(config->emergency_types_array[i].emergency_desc);
            }
            config->emergency_types_array[i].emergency_desc = NULL;


            if(config->emergency_types_array[i].rescuers){
                free(config->emergency_types_array[i].rescuers);
                config->emergency_types_array[i].rescuers = NULL;
            }
            config->emergency_types_array[i].rescuer_required_number = 0;
        }
        free(config->emergency_types_array);
        config->emergency_types_array = NULL;

        sprintf(log_msg_buffer, "Deallogati %d tipi di emergenze", config->emergency_type_num);
        log_message(LOG_EVENT_GENERAL_INFO, "System_config", log_msg_buffer);    
        config->emergency_type_num = 0;
    }

    
    
    config->total_digital_twin_da_creare = 0;
    log_message(LOG_EVENT_GENERAL_INFO, "System_config", "Deallogazione system_config finita");
}