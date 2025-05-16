#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h> //torna un po' meglio per i found
#include<ctype.h>  //serve per isspace
#include<errno.h>


#include "Syscalls_3_progetto.h"
#include"parser.h"
#include"data_types.h"
#include"config1.h"
#include "logger.h"

//Ho provato ad implementare un controllo su spazi e tab aggiunti + righe vuote
//funzione principale è booleana in modo tale da poter gestire altrove eventuale errore

static void rimuovi_spazi(char* str){
    if (!str){
        //ergo se puntatore è nullo
        return;
    }

    //prima guardo se ci sono spazi iniziali
    while(isspace((unsigned char)*str)){    //isspace prende un intero positivo
        str++;//aumento finche non trovo uno spazio o tab o \0
    }

    if(*str == 0){  //terminatore nullo
        return;     //stringa vuota
    }

    //tolgo spazi in fondo
    char *fine;
    fine = str+strlen(str)-1;   //ultimo carattere escluso \0
    while(isspace((unsigned char)*fine) && (fine > str)){
        fine--; //faccio salire fine finche trova spazi e non tocca str
    }
    fine[1] = '\0'; //metto terminatore dopo fine
}

bool parse_environment(const char* nome_file, environment_t *env_config){

    char log_msg_buffer[LINE_LENGTH + 200]; //buffer per messaggi di log formattati

    if(!nome_file || !env_config){
        sprintf(log_msg_buffer, "Parsing fallito per parametri nulli");
        log_message(LOG_EVENT_FILE_PARSING, "parse_environment", log_msg_buffer);
        return false;   //non faccio exit, ho bisogno di sapere se la funzione è andaa a buon fine
    }

    FILE* file;
    sprintf(log_msg_buffer, "Errore fopen apertura file '%s': %s", nome_file, strerror(errno));
    LOG_SNCALL(file, fopen(nome_file, "r"), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    int num_riga = 0;  //serve per file di logging
    char riga[LINE_LENGTH];
    bool coda_trovata = false;
    bool height_trovata = false;
    bool width_trovata = false;
    //utilizzo questi 3 booleani per fare da flag se sono stati trovati i dati

    env_config->queue_name[0] = '\0';
    env_config->grid_height = -1;
    env_config->grid_width = -1;
    //inizializzo tutto
    
    while(fgets(riga, sizeof(riga), file)){
        num_riga++; 

        char riga_copia[LINE_LENGTH];   //faccio una copia per fare logging
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

        //divido all'=
        char* nome = strtok(riga, "=");
        char* valore= strtok(NULL, "= ");   //null significa che proseguo da dove ho tagliato prima

        if(nome && valore){
            rimuovi_spazi(nome);
            rimuovi_spazi(valore);

            if(strcmp(nome, "queue") == 0){
                //presuppongo che lo / lo metto io da nome dato
                //quindi non ne devo avere altri
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

    //controllo se ho trovato tutti e tre i valori che mi servono
    if(!coda_trovata || !height_trovata || !width_trovata){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Errore, uno dei tre parametri (nome coda, altezza, larghezza) non è stato trovato");
        return false;
    }

    if(strlen(env_config->queue_name) == 0) {
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Errore, il nome della coda non può essere vuoto");
        //perchè se vuota comunque avrei coda_trovata a true
        return false;
    }

    sprintf(log_msg_buffer, "Parsing completato: Nome coda=%s, Altezza=%d, Larghezza=%d", env_config->queue_name, env_config->grid_height, env_config->grid_width);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    return true;
    //test env
}