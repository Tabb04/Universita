#define _POSIX_C_SOURCE 200809L 
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

static char* rimuovi_spazi_ptr(char* s){
    if(!s){
        return NULL;
    }

    s[strcspn(s, "\n")] = 0;    
    while(isspace((unsigned char)*s)) s++;  

    if(*s==0){
        return s;   
    }

    char* fine = s + strlen(s) - 1;
    while((fine>=s) && (isspace((unsigned char)*fine))) fine --;
    fine[1] = '\0';
    return s;   
}



static rescuer_type_t* trova_soccorritore(const char* name, const system_config_t* config){
    
    for(int i = 0; i<config->rescuer_type_num; i++){
        if(strcmp(config->rescuers_type_array[i].rescuer_type_name, name) == 0){
            return &config->rescuers_type_array[i];
            
        }
    }
    return NULL;
}


bool parse_emergency_types(const char* nome_file, system_config_t* config){
    
    char log_msg_buffer[LINE_LENGTH + 300];

    if(!nome_file || !config){
        sprintf(log_msg_buffer, "Parsing fallito per parametri nulli");
        log_message(LOG_EVENT_FILE_PARSING, "parse_rescuers", log_msg_buffer);
        return false;
    }
    
    
    if((config->rescuer_type_num > 0) && (!config->rescuers_type_array)) {
        sprintf(log_msg_buffer, "Errore in configurazione, numero tipi di soccorritori = %d ma array invalido", config->rescuer_type_num);
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false;
    }

    sprintf(log_msg_buffer, "Tentativo apertura del file '%s'", nome_file);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    FILE* file;
    sprintf(log_msg_buffer, "Errore fopen apertura file '%s': %s", nome_file, strerror(errno));
    LOG_SNCALL(file, fopen(nome_file, "r"), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    
    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    
    char riga[LINE_LENGTH];
    int riga_num_pass1 = 0;
    int type_count = 0;

    
    

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Primo passaggio: Conto pontenziali tipi di emergenze");
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

        char nome_temp[EMERGENCY_NAME_LENGTH]; 
        short prior_temp; 
        if(sscanf(riga_copia, " [%[^]]] [%hd]", nome_temp, &prior_temp) >=2){
            type_count++;   
        }
    }
    sprintf(log_msg_buffer, "Primo passsaggio: Completato, trovati potenziali %d tipi di emergenze", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    
    if(type_count == 0){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Nessun tipo di emergenza trovato");
        config->emergency_type_num = 0;
        config->emergency_types_array = NULL;
        if(fclose(file) == EOF){
            sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
            file = NULL;
        }
        return true;    
    }
    
    config->emergency_types_array = (emergency_type_t*)malloc(type_count * sizeof(emergency_type_t));

    if(!config->emergency_types_array){
        
        sprintf(log_msg_buffer, "Errore su malloc su allocazione per %d tipi di emergenze: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        if(fclose(file) == EOF){
            sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
            file = NULL;
        }
        return false;
    }

    sprintf(log_msg_buffer, "Allocato spazio per %d emergenze", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    
    
    config->emergency_type_num = 0;

    rewind(file);
    int riga_num_pass2 = 0;
    int indice_emergenza = 0;

    
    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Secondo passsaggio: Estraggo dati");

    while((fgets(riga, sizeof(riga), file)) && (indice_emergenza < type_count)){
        riga_num_pass2++;

        
        char riga_copia1[LINE_LENGTH];
        strncpy(riga_copia1, riga, LINE_LENGTH - 1);
        riga_copia1[LINE_LENGTH - 1 ] = '\0';
        riga_copia1[strcspn(riga_copia1, "\n")] = '\0';

        
        char riga_copia_parsing[LINE_LENGTH];
        strcpy(riga_copia_parsing, riga);

        char* riga_ptr = rimuovi_spazi_ptr(riga_copia_parsing);
        if(strlen(riga_ptr) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        
        
        char nome_temp1[EMERGENCY_NAME_LENGTH];
        short prior_temp1;
        char riga_intera_soccorritori[LINE_LENGTH] = "";

        
        if(*riga_ptr != '['){   
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        riga_ptr++;

        char* riga_fine_nome=strchr(riga_ptr, ']'); 
        if((!riga_fine_nome) || (riga_fine_nome == riga_ptr) || ((riga_fine_nome - riga_ptr) >= EMERGENCY_NAME_LENGTH)){
            
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        strncpy(nome_temp1, riga_ptr, riga_fine_nome - riga_ptr);
        nome_temp1[riga_fine_nome - riga_ptr] = '\0';
        
        
        riga_ptr = riga_fine_nome + 1;


        
        while(isspace((unsigned char)*riga_ptr)){
            riga_ptr++; 
        }

        if(*riga_ptr != '['){   
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        riga_ptr++;

        char* riga_fine_prior=strchr(riga_ptr, ']'); 
        if(!riga_fine_prior){
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        char buff_prio[10];
        if((riga_fine_prior == riga_ptr) || ((size_t)(riga_fine_prior - riga_ptr) > sizeof(buff_prio))){    
            
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        strncpy(buff_prio, riga_ptr, riga_fine_prior - riga_ptr);
        buff_prio[riga_fine_prior - riga_ptr] = '\0';

        if(sscanf(buff_prio, "%hd", &prior_temp1) != 1){
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
    
        
        riga_ptr = riga_fine_prior + 1;

        
        while(isspace((unsigned char)*riga_ptr)) riga_ptr++; 

        if(*riga_ptr != '\0') { 
            strncpy(riga_intera_soccorritori, riga_ptr, sizeof(riga_intera_soccorritori)-1);
            riga_intera_soccorritori[sizeof(riga_intera_soccorritori)-1] = '\0'; 
        }

        
        if((prior_temp1 <BASSA_PRIOR) | (prior_temp1 > ALTA_PRIOR)){
            sprintf(log_msg_buffer, "Errore riga %d priorità fuori dal range: '%s'", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        if(strlen(riga_intera_soccorritori) == 0){
            sprintf(log_msg_buffer, "Errore riga %d, elenco soccorritori mancante per emergenza %s: '%s'", riga_num_pass2, nome_temp1, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        
        

        int req_array_cap = 5;  
        
        rescuer_request_t *request_arr = malloc(req_array_cap * sizeof(rescuer_request_t));

        if(!request_arr){   
            sprintf(log_msg_buffer, "Errore riga %d, impossibile allocare spazio per soccorritori", riga_num_pass2);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            if(fclose(file) == EOF){
                sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                file = NULL;
            }
            
            for(int i = 0; i<indice_emergenza; i++){
                if(config->emergency_types_array[i].emergency_desc){
                    free(config->emergency_types_array[i].emergency_desc);
                }
                if(config->emergency_types_array[i].rescuers){
                    free(config->emergency_types_array[i].rescuers);
                }
            }
            if(config->emergency_types_array){
                free(config->emergency_types_array);
            }
            config->emergency_types_array = NULL;
            config->emergency_type_num = 0;
            return false;
        }
        int req_count_vero = 0;   

        
        char* current_rescuer_token;    
        char *riga_soccorritori_resto = riga_intera_soccorritori; 
        char *token_dopo; 

        
        

        while((current_rescuer_token = strtok_r(riga_soccorritori_resto, ";", &token_dopo)) != NULL){
            
            
            
            riga_soccorritori_resto = NULL; 
            

            
            char token_copia_da_pulire[LINE_LENGTH];
            strncpy(token_copia_da_pulire, current_rescuer_token, LINE_LENGTH -1);
            
            token_copia_da_pulire[LINE_LENGTH - 1] = '\0';

            
            char* token_pulito = rimuovi_spazi_ptr(token_copia_da_pulire);

            if(strlen(token_pulito) == 0){
                continue;
            }

            
            char rt_name_buf[RESCUER_NAME_LENGTH];
            int num_needed_val, time_needed_val;

            if(sscanf(token_pulito, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3){
                sprintf(log_msg_buffer, "Errore riga %d, formato richiesta soccorritore non valido '%s'", riga_num_pass2, token_pulito);
                log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                continue;
            }
            if((num_needed_val <=0) || (time_needed_val <=0) || (strlen(rt_name_buf) == 0)){
                sprintf(log_msg_buffer, "Errore riga %d, Uno o più valori invalidi", riga_num_pass2);
                log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                continue;
            }

            rescuer_type_t* found = trova_soccorritore(rt_name_buf, config);
            if(!found){
                sprintf(log_msg_buffer, "Errore riga %d, tipo di soccorritore '%s' non trovato", riga_num_pass2, rt_name_buf);
                log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                continue;               
            }

            if(req_count_vero >= req_array_cap){
                req_array_cap *=2;
                rescuer_request_t* request_array_temp = realloc(request_arr, req_array_cap * sizeof(rescuer_request_t));
                if(!request_array_temp){
                    sprintf(log_msg_buffer, "Errore riallocazione memoria per '%s': %s", nome_temp1, strerror(errno));
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                    
                    free(request_arr);
                    if(fclose(file) == EOF){
                        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                        file = NULL;
                    }
                    for (int i = 0; i < indice_emergenza; ++i) {
                        if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
                        if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
                    }
                    if(config->emergency_types_array) free(config->emergency_types_array);
                    config->emergency_types_array = NULL; config->emergency_type_num = 0;
                    return false;
                }
                request_arr = request_array_temp;
            }
            request_arr[req_count_vero].type = found;
            request_arr[req_count_vero].required_count = num_needed_val;
            request_arr[req_count_vero].time_to_manage = time_needed_val;
            req_count_vero++;
        }

        if(req_count_vero == 0){
            sprintf(log_msg_buffer, "Errore, nessuna richiesta di soccorritore valida trovata per emergenza '%s'", riga_intera_soccorritori);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            free(request_arr);
            continue;
        }

        
        emergency_type_t *emergency_types_ptr = &config->emergency_types_array[indice_emergenza];
        
        
        emergency_types_ptr->emergency_desc = (char*)malloc(strlen(nome_temp1) + 1);
        if(!emergency_types_ptr->emergency_desc){
            sprintf(log_msg_buffer, "Errore, fallimento allocazione memoria per emergenza '%s'", nome_temp1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            free(request_arr);
            for(int i = 0; i < indice_emergenza; ++i) {
                if(config->emergency_types_array[i].emergency_desc) {
                    free(config->emergency_types_array[i].emergency_desc);
                }
                if(config->emergency_types_array[i].rescuers) {
                    free(config->emergency_types_array[i].rescuers);
                }
            }   
            if(config->emergency_types_array) free(config->emergency_types_array);
            config->emergency_types_array = NULL;
            config->emergency_type_num = 0;
            if(fclose(file) == EOF){
                sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                file = NULL;
            }
            return false;
        }
        strcpy(emergency_types_ptr->emergency_desc, nome_temp1);
        
        
        emergency_types_ptr->priority = prior_temp1;
        emergency_types_ptr->rescuers = request_arr;
        emergency_types_ptr->rescuer_required_number = req_count_vero;

        
        sprintf(log_msg_buffer, "Caricata riga n* %d: Nome=%s, Priorità=%hd, Soccorsi richiesti=%d", riga_num_pass2, emergency_types_ptr->emergency_desc, emergency_types_ptr->priority, emergency_types_ptr->rescuer_required_number);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

        
        for(int i=0; i<emergency_types_ptr->rescuer_required_number; i++){
            sprintf(log_msg_buffer, "Soccorso=%s, Numero unità=%d, Tempo di gestine=%d", request_arr[i].type->rescuer_type_name, request_arr[i].required_count, request_arr[i].time_to_manage);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        }
        indice_emergenza++;
        continue;
    }

    
    config->emergency_type_num = indice_emergenza;

    if(fclose(file) == EOF){
        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
        file = NULL;
    }

    
    if((config->emergency_type_num == 0) && (type_count > 0)){
        sprintf(log_msg_buffer, "Nessuna emergenza valida estratta da %d potenziali", type_count);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        
        
        if(config->emergency_types_array){
            free(config->emergency_types_array);
        }
        config->emergency_types_array = NULL;
        return false;
    }
    

    sprintf(log_msg_buffer, "Completato parsing. Letti %d emergenze valide", config->emergency_type_num);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    return true;
}