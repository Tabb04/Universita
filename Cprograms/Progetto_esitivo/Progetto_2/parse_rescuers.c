#include "parser.h"
#include "data_types.h"
#include "config1.h"
#include "logger.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <Syscalls_3_progetto.h>

//stessa roba di parse_env


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

    //per allocare nel mio system_config_t rescuer_types_array, devo sapere prima quanti sono

    char riga[LINE_LENGTH];     //stesso per leggere una riga alla volta
    int riga_num_pass1 = 0;     //contatore righe al primo passaggio
    int type_count = 0;         //contatore dei tipi di soccorritori potenzialmente validi

    log_message(LOG_EVENT_FILE_PARSING, "Primo passaggio: Conto pontenziali tipi di soccorritori", log_msg_buffer);
    while(fgets(riga, sizeof(riga), file)){
        riga_num_pass1++;
        
        char riga_copia[LINE_LENGTH];   //intanto copio per sicurezza perchè modifico in place
        strncpy(riga_copia, riga, LINE_LENGTH - 1);
        riga_copia[LINE_LENGTH - 1] = '\0';
        riga_copia[strcspn(riga_copia, '\n')] = 0;  //strcspn trova pos del \n e lo toglie
        rimuovi_spazi(riga_copia);

        if(strlen(riga_copia) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //per conteggio copio in variabili temporanee
        char temp_nome[RESCUER_NAME_LENGTH];
        int temp_num;
        int temp_speed;
        int temp_x;
        int temp_y;

        if(sscanf(riga_copia, " [%[^]]] [%d] [%d] [%d;%d]", temp_nome, &temp_num, &temp_speed, &temp_x, &temp_y) == 5){
            //guardo solo se ha una formattazione accettabile
            //[^]] leggi una sequenza finche non trovi una parentesi chiusa
            type_count++;
        }
    }

    sprintf(log_msg_buffer, "Primo passsaggio: Completato, trovati potenziali %d tipi di soccorritori", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    //nel caso non venga trovato nessun tipo
    if(type_count == 0){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Nessun tipo di soccorritore trovato");
        config->rescuer_type_num = 0;
        config->rescuers_type_array = NULL;
        config->total_digital_twin_da_creare = 0;
        config->instances_per_rescuer_type = NULL;
        fclose(file);
        return true;    //vedo poi come gestirlo perchè il file potrebbe essere in effetti vuoto?
    }

    //utilizzo type_cont per intanto farmi un array sicuramente abbastanza grande
    config->rescuers_type_array = (rescuer_type_t *)malloc(type_count * sizeof(rescuer_type_t));
    
    if(!config->rescuers_type_array){
        //in teoria malloc non è una scall quindi non uso macro
        sprintf(log_msg_buffer, "Errore su malloc su allocazione per %d tipi di soccorritore: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        fclose(file);
        return false;
    }
    sprintf(log_msg_buffer, "Allocato spazio per %d soccorritori", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);


    config->instances_per_rescuer_type = malloc(type_count * sizeof(int));
    if (!config->instances_per_rescuer_type) {
        sprintf(log_msg_buffer, "Errore su malloc per instances_per_rescuer_type per %d interi: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        free(config->rescuers_type_array); //libero array principlae allocato
        config->rescuers_type_array = NULL;
        fclose(file);
        return false;
    }
    sprintf(log_msg_buffer, "Allocato instances_per_rescuer_type per %d interi.", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);


    //inizializzo contatori
    config->total_digital_twin_da_creare = 0;
    config->rescuer_type_num = 0;

    //ora faccio l'effettiva estrazione di dati
    rewind(file);
    int riga_num_pass2 = 0;
    int indice_tipo = 0;
    //lo utilizzo anche come controllo ridondante se non ho superato type count


    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Secondo passsaggio: Estraggo dati");

    while(fgets(riga, sizeof(riga), file) && indice_tipo < type_count){
        riga_num_pass2++;
        
        //stessa roba come sopra
        char riga_copia1[LINE_LENGTH];  //me la tengo più per il logging alla fine che per altro
        strncpy(riga_copia1, riga, LINE_LENGTH -1);
        riga_copia1[LINE_LENGTH - 1] = '\0';
        riga_copia1[strcspn(riga_copia1, "\n")] = '\0';

        riga[strcspn(riga, "\n")] = '\0';
        rimuovi_spazi(riga);

        if(strlen(riga) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass2);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //variabili temporanee 2 per salvare i dati
        char nome[RESCUER_NAME_LENGTH];
        int num;    //quante unità per questo tipo
        int speed;
        int base_x;
        int base_y;

        int found = sscanf(riga, " [%[^]]] [%d] [%d] [%d;%d]", nome, &num, &speed, &base_x, &base_y);
        //diviso in due per gestirlo meglio
        if(found != 5){
            //son sicuro che c'è un formato errato
            sprintf(log_msg_buffer, "Errore riga %d, riga malformata -> '%s'", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //faccio tutti i controlli che mi vengono in mente
        if(num <= 0 || speed <= 0 || base_x <= 0 || base_y <=0 || strlen(nome)==0 || strlen(nome) >= RESCUER_NAME_LENGTH){
            sprintf(log_msg_buffer, "Errore riga %d, uno o più valori non validi -> (num=%d, speed=%d, base_x=%d, base_y=%d, lunghezza_nome=%zu), '%s'", riga_num_pass2, num, speed, base_x, base_y, strlen(nome), riga_copia1);
            //%zu per le dimensioni si usa
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        
        rescuer_type_t *current = &config->emergency_types_array[indice_tipo];
        //current ora punta al primo elemento dell'array dei tipi

        current->rescuer_type_name = (char*)malloc(strlen(nome) + 1);
        if(current->rescuer_type_name != NULL){
            //se malloc ha funzionato ci scrivo
            strcpy(current->rescuer_type_name, nome);
        }else{
            sprintf(log_msg_buffer, "Errore riga %d, fallita allocazione memoria -> nome soccorritore=%s, '%s'", nome, strerror(errno));
            //in teoria malloc setta errno
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

            for(int i = 0; i<indice_tipo; i++){
                if(config->rescuers_type_array[i].rescuer_type_name){
                    free(config->rescuers_type_array[i].rescuer_type_name);
                }
            }

            free(config->rescuers_type_array); 
            config->rescuers_type_array = NULL;
            config->rescuer_type_num = 0;
            fclose(file);
            return false;
        }
        //in teoria non possono esserci altri errori quindi assegno valore
        current->speed = speed;
        current->x = base_x;
        current->y = base_y;
        config->total_digital_twin_da_creare += num;
        config->instances_per_rescuer_type[indice_tipo] = num;

        sprintf(log_msg_buffer, "Estratto soccorritore da riga %d: Nome=%s, Velocità=%d, Base=(%d,%d). Numero unità=%d", riga_num_pass2, current->rescuer_type_name, current->speed, current->x, current->y, num);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

        indice_tipo++;
    }

    config->rescuer_type_num = indice_tipo;   //se tutto bene è il numero di cicli
    fclose(file);

    //controllo in +, tutte le righe erano formattate male
    if(config->rescuer_type_num == 0 && type_count > 0){
        sprintf(log_msg_buffer, "Trovate %d righe potenziali ma nessuna convalidata", type_count);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        free(config->rescuers_type_array); //non devo liberare nomi tanto non ne ho allocati
        config->rescuers_type_array = NULL;
        return false;   //inutile continuare chiaramente errato, non come file vuoto che potrebbe essere sensato
    }

    //se sono qui tutto è andato bene
    sprintf(log_msg_buffer, "Parsing dei soccorritori completato. Letti %d tipi di soccorritori, vanno creati totali %d gemelli virtuali", config->rescuer_type_num, config->total_digital_twin_da_creare);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    return true;
}



//metto qui free_system_config

void free_system_config(system_config_t* config){
    char log_msg_buffer[200];

    if(!config){
        return; //non libero nulla se è gia NULL
    }

    log_message(LOG_EVENT_GENERAL_INFO, "System_config", "Inizio deallocazione system_config");


    //prima libero soccorritori
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

    //libero numero di tipi di soccorritori
    if(config->instances_per_rescuer_type) {
        free(config->instances_per_rescuer_type);
        config->instances_per_rescuer_type = NULL;
        sprintf(log_msg_buffer, "Deallocato instances_per_rescuer_type.");
        log_message(LOG_EVENT_GENERAL_INFO, "System_config", log_msg_buffer);
    }


    //poi libero emergenze
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

    //test resc
    
    config->total_digital_twin_da_creare = 0;
    log_message(LOG_EVENT_GENERAL_INFO, "System_config", "Deallogazione system_config finita");
}