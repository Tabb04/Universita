#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h> 
#include<ctype.h>  
#include<errno.h>


#include "Syscalls_3_progetto.h"
#include"parser.h"
#include"data_types.h"
#include"config1.h"
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

bool parse_environment(const char* nome_file, environment_t *env_config){

    char log_msg_buffer[LINE_LENGTH + 200]; 

    if(!nome_file || !env_config){
        sprintf(log_msg_buffer, "Parsing fallito per parametri nulli");
        log_message(LOG_EVENT_FILE_PARSING, "parse_environment", log_msg_buffer);
        return false;   
    }

    FILE* file;
    sprintf(log_msg_buffer, "Errore fopen apertura file '%s': %s", nome_file, strerror(errno));
    LOG_SNCALL(file, fopen(nome_file, "r"), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    int num_riga = 0;  
    char riga[LINE_LENGTH];
    bool coda_trovata = false;
    bool height_trovata = false;
    bool width_trovata = false;
    

    env_config->queue_name[0] = '\0';
    env_config->grid_height = -1;
    env_config->grid_width = -1;
    
    
    while(fgets(riga, sizeof(riga), file)){
        num_riga++; 

        char riga_copia[LINE_LENGTH];   
        strncpy(riga_copia, riga, LINE_LENGTH - 1);
        riga_copia[LINE_LENGTH - 1] = '\0';
        riga_copia[strcspn(riga_copia, "\n")] = '\0';

        riga[strcspn(riga, "\n")] = '\0';
        rimuovi_spazi(riga);

        if(strlen(riga) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", num_riga);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        
        char* nome = strtok(riga, "=");
        char* valore= strtok(NULL, "= ");   

        if(nome && valore){
            rimuovi_spazi(nome);
            rimuovi_spazi(valore);

            if(strcmp(nome, "queue") == 0){
                
                
                if((strlen(valore) >= sizeof(env_config->queue_name)) || (strchr(valore, '/'))  ){
                    sprintf(log_msg_buffer, "Errore riga %d, nome troppo lungo o contentente carattere '\'", num_riga);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                }else{
                    strncpy(env_config->queue_name, valore, sizeof(env_config->queue_name) - 1);
                    env_config->queue_name[sizeof(env_config->queue_name) - 1] = '\0';
                    coda_trovata = true;
                    sprintf(log_msg_buffer, "Coda trovata a riga %d: '%s'", num_riga, env_config->queue_name);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                }

            }else if(strcmp(nome, "height") == 0){
                if(sscanf(valore, "%d", &env_config->grid_height) != 1 || env_config->grid_height <= 0){
                    sprintf(log_msg_buffer, "Errore riga %d, valore per height=%s non valido", num_riga, valore);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                    if(fclose(file) == EOF){
                        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                        file = NULL;
                    }
                    return false;
                }else{
                    height_trovata = true;
                    sprintf(log_msg_buffer, "Altezza trovata a riga %d: '%d'", num_riga, env_config->grid_height);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                }
            }else if(strcmp(nome, "width") == 0){
                if(sscanf(valore, "%d", &env_config->grid_width) != 1 || env_config->grid_width <= 0){
                    sprintf(log_msg_buffer, "Errore riga %d, valore per width=%s non valido", num_riga, valore);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                    if(fclose(file) == EOF){
                        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
                        file = NULL;
                    }
                    return false;
                }else{
                    width_trovata = true;
                    sprintf(log_msg_buffer, "Ampiezza trovata a riga %d: '%d'", num_riga, env_config->grid_width);
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                }
            }else{
                sprintf(log_msg_buffer, "Errore riga %d, chiave non riconosciuta per '%s'", num_riga, riga_copia);
                log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            }
        }else{
            sprintf(log_msg_buffer, "Errore riga %d, formato non valido (chiave=valore): '%s'", num_riga, riga_copia);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        }
    }
    
    if(fclose(file) == EOF){
        sprintf(log_msg_buffer, "Errore durante la chiusura del file '%s': %s", nome_file, strerror(errno));
        file = NULL;
    }

    
    if(!coda_trovata || !height_trovata || !width_trovata){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Errore, uno dei tre parametri (nome coda, altezza, larghezza) non è stato trovato");
        return false;
    }

    if(strlen(env_config->queue_name) == 0) {
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Errore, il nome della coda non può essere vuoto");
        
        return false;
    }

    sprintf(log_msg_buffer, "Parsing completato: Nome coda=%s, Altezza=%d, Larghezza=%d", env_config->queue_name, env_config->grid_height, env_config->grid_width);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    return true;
    
}