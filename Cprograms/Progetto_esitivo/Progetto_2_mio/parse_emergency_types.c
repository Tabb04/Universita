#include "parser.h"
#include "data_types.h"
#include "config1.h"
#include "logger.h" // Includi il logger
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <Syscalls_3_progetto.h>


//Faccio gli stessi controlli di env e rescuers
//non do per presupposto che i soccorsi richiesti siano presenti
//es. chiedo pompieri ma ho solo polizia e ambulanze

//sempre stessa funzione
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

static char* rimuovi_spazi_ptr(char* s){
    if(!s){
        return NULL;
    }

    s[strcspn(s, "\n")] = 0;    //toglie \n
    while(isspace((unsigned char)*s)) s++;  //salta spazi inizio

    if(*s==0){
        return s;   //stringa vuota
    }

    char* fine = s + strlen(s) - 1;
    while((fine>=s) && (isspace((unsigned char)*fine))) fine --;
    fine[1] = '\0';
    return s;   //restituisco la stringa qui
}


//funzione di supporto usata sotto per vedere se ho quel tipo di soccorritore
static rescuer_type_t* trova_soccorritore(const char* name, const system_config_t* config){
    //qui presuppongo siano già stati parsati i soccorritori ovviamente
    for(int i = 0; i<config->rescuer_type_num; i++){
        if(strcmp(config->rescuers_type_array[i].rescuer_type_name, name) == 0){
            return &config->emergency_types_array[i];
            //restituisco putnatore a quella struttura
        }
    }
    return NULL;
}


bool parse_emergency_types(const char* nome_file, system_config_t* config){
    //solito buffer
    char log_msg_buffer[LINE_LENGTH + 300];

    if(!nome_file || !config){
        sprintf(log_msg_buffer, "Parsing fallito per parametri nulli");
        log_message(LOG_EVENT_FILE_PARSING, "parse_rescuers", log_msg_buffer);
        return false;
    }
    
    //in teoria improbabile ma controllo se si fosse invalidato l'array dei tipi
    if((config->rescuer_type_num > 0) && (!config->rescuers_type_array)) {
        sprintf(log_msg_buffer, "Errore in configurazione, numero tipi di soccorritori = %d ma array invalido", config->rescuer_type_num);
        log_message(LOG_EVENT_FILE_PARSING, "parse_emergency_types", log_msg_buffer);
        return false;
    }

    sprintf(log_msg_buffer, "Tentativo apertura del file '%s'", nome_file);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    FILE* file;
    LOG_SNCALL(file, fopen(nome_file, "r"), (log_msg_buffer, "Errore fopen su '%s': %s", nome_file, strerror(errno)), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    //variabili primo ciclo
    char riga[LINE_LENGTH];
    int riga_num_pass1 = 0;
    int type_count = 0;

    //per primo faccio stesso passaggio di parse_rescuers dove conto i potenziali per 
    //allocare sufficiente memoria 

    log_message(LOG_EVENT_FILE_PARSING, "Primo passaggio: Conto pontenziali tipi di emergenze", log_msg_buffer);
    while(fgets(riga, sizeof(riga), file)){
        riga_num_pass1++;
        
        //anche qui per sicurezza copio riga perchè modifico in place
        char riga_copia[LINE_LENGTH];
        strncpy(riga_num_pass1, riga, LINE_LENGTH - 1);
        riga[LINE_LENGTH - 1] = '\0';
        riga_copia[strcspn(riga_copia, "\n")] = 0; //tolgo \n
        rimuovi_spazi(riga_copia);

        if(strlen(riga_copia) == 0){
            sprintf(log_message, "Riga %d vuota, ignoro", riga_num_pass1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        char nome_temp[EMERGENCY_NAME_LENGTH]; //riga temporanea per salvare nome
        short prior_temp; //come dice pdf
        if(sscanf(riga_copia, " [%[^]]] [%hd]", nome_temp, &prior_temp) >=2){
            type_count++;   //se riesce a trovarli entrambi aumento il counter
        }
    }
    sprintf(log_msg_buffer, "Primo passsaggio: Completato, trovati potenziali %d tipi di emergenze", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    //nel caso non venga trovato nessun tipo
    if(type_count == 0){
        log_message(LOG_EVENT_FILE_PARSING, nome_file, "Nessun tipo di emergenza trovato");
        config->emergency_type_num = 0;
        config->emergency_types_array = NULL;
        fclose(file);
        return true;    //anche qui considero file vuoto come potenzialmente corretto
    }
    
    config->emergency_types_array = (emergency_type_t*)malloc(type_count * sizeof(emergency_t));

    if(!config->emergency_types_array){
        //malloc non è syscall quindi gestisco nello stesso modo di parse_rescuers
        sprintf(log_msg_buffer, "Errore su malloc su allocazione per %d tipi di emergenze: '%s'", type_count, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        fclose(file);
        return false;
    }

    sprintf(log_msg_buffer, "Allocato spazio per %d emergenze", type_count);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    
    //inizializzo a 0
    config->emergency_type_num = 0;

    rewind(file);
    int riga_num_pass2 = 0;
    int indice_emergenza = 0;

    //qui effettiva estrazione e convalidazione dei valori
    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Secondo passsaggio: Estraggo dati");

    while((fgets(riga, sizeof(riga), file)) && (indice_emergenza < type_count)){
        riga_num_pass2++;

        //faccio due copie, una per logging e una perchè uso strtok (per sicurezza)
        char riga_copia1[LINE_LENGTH];
        strncpy(riga_copia1, riga, LINE_LENGTH - 1);
        riga_copia1[LINE_LENGTH - 1 ] = '\0';
        riga_copia1[strcspn(riga_copia1, "\n")] = '\0';

        //ora una copia su cui lavorare, e mi muovo tramite un puntatore (uso funzione di puliza 2)
        char riga_copia_parsing[LINE_LENGTH];
        strcpy(riga_copia_parsing, riga);

        char* riga_ptr = rimuovi_spazi_ptr(riga_copia_parsing);
        if(strlen(riga_ptr) == 0){
            sprintf(log_message, "Riga %d vuota, ignoro", riga_num_pass1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        
        //variabili temporanee
        char nome_temp1[EMERGENCY_NAME_LENGTH];
        short prior_temp1;
        char riga_intera_soccorritori[LINE_LENGTH] = "";

        //ESTRAGGO NOME
        if(*riga_ptr != '['){   //deve per forza iniziare con [
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        riga_ptr++;

        char* riga_fine_nome=strchr(riga_ptr, ']'); //trova la ] di chiusura del nome
        if((!riga_fine_nome) || (riga_fine_nome == riga_ptr) || ((riga_fine_nome - riga_ptr) >= EMERGENCY_NAME_LENGTH)){
            //controllo se c'è fine, se non sono vuote le parentesi e se è più corto di emergency_name_length
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //qui posso copiare
        strncpy(nome_temp1, riga_ptr, riga_fine_nome - riga_ptr);
        nome_temp1[riga_fine_nome - riga_ptr] = '\0';
        
        //ora metto il puntatore dopo la [
        riga_ptr = riga_fine_nome + 1;


        //ESTRAGGO PRIORITA
        while(isspace((unsigned char)*riga_ptr)){
            riga_ptr++; //salto se ci sono spazi prima
        }

        if(*riga_ptr != '['){   //deve per forza iniziare con [
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
        riga_ptr++;

        char* riga_fine_prior=strchr(riga_ptr, ']'); //trova la ] di chiusura del nome
        if(!riga_fine_prior){
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //mi faccio una stringa buffer per priorità
        char buff_prio[10];
        if((riga_fine_prior == riga_ptr) || ((riga_fine_prior - riga_ptr) > sizeof(buff_prio))){
            //priorià vuota o stringa troppo grossa
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }

        //copio stringa della priorità
        strncpy(buff_prio, riga_ptr, riga_fine_prior - riga_ptr);
        buff_prio[riga_fine_prior - riga_ptr] = '\0';

        if(sscanf(buff_prio, "%hd", &prior_temp1) != 1){
            sprintf(log_msg_buffer, "Errore riga %d, formato errato: '%s", riga_num_pass2, riga_copia1);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            continue;
        }
    
        //metto puntatore dopo ]
        riga_ptr = riga_fine_prior + 1;

        //ESTRAGGO SOCCORRITORI E RESTO
        while(isspace((unsigned char)*riga_ptr)) riga_ptr++; //spazi iniziali

        if(*riga_ptr != '\0') { //se non vuota
            strncpy(riga_intera_soccorritori, riga_ptr, sizeof(riga_intera_soccorritori)-1);
            riga_intera_soccorritori[sizeof(riga_intera_soccorritori)-1] = '\0'; 
        }

        //validazione valori
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

        //parsing della stringa dei soccorritori, a questo punto ho una roba del genere
        //Pompieri:2,10;Ambulanza:1,5;Polizia:1,3
        //dove [Nome_emergenza] [Priorità] è già stata processata

        int req_array_cap = 5;  //per ora alloco spazio 5 per cominciare
        //sarei in emergency_type in -> rescuer_request_t *rescuers;
        rescuer_request_t *request_arr = malloc(req_array_cap * sizeof(rescuer_request_t));

        if(!request_arr){   //se fallita malloc
            sprintf(log_msg_buffer, "Errore riga %d, impossibile allocare spazio per soccorritori", riga_num_pass2);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
            fclose(file);
            //se non ero alla prima iterazione libero array
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
        int actual_req_count = 0;   //salvo quanti campi effettivamente ho, probabile non 5

        //creo due variabili per navigare quando faccio strtok_r
        char* current_rescuer_token;    //punta al token corrente
        char *rest_of_rescuers_str = riga_intera_soccorritori; //lavoro su copia perchè modifico
        char *saveptr_token; //puntatore stato interno di strtok_r

        //tokenizzo utilizzando ; come separatore
        //ES. Pompieri:2,10;Ambulanza:1,5;Polizia:1,3 

        while(current_rescuer_token = strtok_r(rest_of_rescuers_str, ";", &saveptr_token) != NULL){
            
            //current_rescuer_token punta alla P di pompieri
            //saveptr_token punta a A di ambulanza
            rest_of_rescuers_str = NULL; //per chiamate dopo strtok_r, continua da dove era prima
            
            //uso stringa di supporto
            char token_copy_for_trim[LINE_LENGTH];
            strncpy(token_copy_for_trim, current_rescuer_token, LINE_LENGTH -1);
            token_copy_for_trim[LINE_LENGTH - 1] = '\0';

            //poi un puntatore perchè uso la funzione di pulizia 2
            char* trimmed_token = rimuovi_spazi_ptr(token_copy_for_trim);

            if(strlen(trimmed_token) == 0){
                continue;
            }

            //buffer per salvare valori
            char rt_name_buf[RESCUER_NAME_LENGTH];
            int num_needed_val, time_needed_val;

            if(sscanf(trimmed_token, "%[^:]:%d,%d", rt_name_buf, &num_needed_val, &time_needed_val) != 3){
                sprintf(log_msg_buffer, "Errore riga %d, formato richiesta soccorritore non valido '%s'", riga_num_pass2, trimmed_token);
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

            if(actual_req_count >= req_array_cap){
                req_array_cap *=2;
                rescuer_request_t* temp_req_array = realloc(request_arr, req_array_cap * sizeof(rescuer_request_t));
                if(!temp_req_array){
                    sprintf(log_msg_buffer, "Errore riallocazione memoria per '%s': %s", nome_temp1, strerror(errno));
                    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
                    
                    free(request_arr);
                    fclose(file);
                    for (int i = 0; i < indice_emergenza; ++i) {
                        if (config->emergency_types_array[i].emergency_desc) free(config->emergency_types_array[i].emergency_desc);
                        if (config->emergency_types_array[i].rescuers) free(config->emergency_types_array[i].rescuers);
                    }
                    if(config->emergency_types_array) free(config->emergency_types_array);
                    config->emergency_types_array = NULL; config->emergency_type_num = 0;
                    return false;
                }
                request_arr = temp_req_array;
            }
            request_arr[actual_req_count].type = found;
            request_arr[actual_req_count].required_count = num_needed_val;
            request_arr[actual_req_count].time_to_manage = time_needed_val;
            actual_req_count++;
        }

        


    }
}