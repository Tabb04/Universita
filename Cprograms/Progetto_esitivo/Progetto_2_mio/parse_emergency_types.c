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
            return &config->rescuers_type_array[i];
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
    sprintf(log_msg_buffer, "Errore fopen apertura file '%s': %s", nome_file, strerror(errno));
    LOG_SNCALL(file, fopen(nome_file, "r"), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    
    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    //variabili primo ciclo
    char riga[LINE_LENGTH];
    int riga_num_pass1 = 0;
    int type_count = 0;

    //per primo faccio stesso passaggio di parse_rescuers dove conto i potenziali per 
    //allocare sufficiente memoria 

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Primo passaggio: Conto pontenziali tipi di emergenze");
    while(fgets(riga, sizeof(riga), file)){
        riga_num_pass1++;
        
        //anche qui per sicurezza copio riga perchè modifico in place
        char riga_copia[LINE_LENGTH];
        strncpy(riga_copia, riga, LINE_LENGTH - 1);
        riga[LINE_LENGTH - 1] = '\0';
        riga_copia[strcspn(riga_copia, "\n")] = 0; //tolgo \n
        rimuovi_spazi(riga_copia);

        if(strlen(riga_copia) == 0){
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass1);
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
    
    config->emergency_types_array = (emergency_type_t*)malloc(type_count * sizeof(emergency_type_t));

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
            sprintf(log_msg_buffer, "Riga %d vuota, ignoro", riga_num_pass1);
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
        int req_count_vero = 0;   //salvo quanti campi effettivamente ho, probabile non 5

        //creo due variabili per navigare quando faccio strtok_r
        char* current_rescuer_token;    //punta al token corrente
        char *riga_soccorritori_resto = riga_intera_soccorritori; //lavoro su copia perchè modifico
        char *token_dopo; //puntatore stato interno di strtok_r

        //tokenizzo utilizzando ; come separatore
        //ES. Pompieri:2,10;Ambulanza:1,5;Polizia:1,3 

        while((current_rescuer_token = strtok_r(riga_soccorritori_resto, ";", &token_dopo)) != NULL){
            
            //current_rescuer_token punta alla P di pompieri e finisce con \0
            //token_dopo punta a A di ambulanza
            riga_soccorritori_resto = NULL; //per chiamate dopo strtok_r, continua da dove era prima
            

            //uso stringa di supporto
            char token_copia_da_pulire[LINE_LENGTH];
            strncpy(token_copia_da_pulire, current_rescuer_token, LINE_LENGTH -1);
            ///ora toke_copy_for_trim contiene Pompieri:2,10
            token_copia_da_pulire[LINE_LENGTH - 1] = '\0';

            //poi un puntatore perchè uso la funzione di pulizia 2
            char* token_pulito = rimuovi_spazi_ptr(token_copia_da_pulire);

            if(strlen(token_pulito) == 0){
                continue;
            }

            //buffer per salvare valori
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
                    fclose(file);
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

        //faccio struttura emergency_type_t
        emergency_type_t *emergency_types_ptr = &config->emergency_types_array[indice_emergenza];
        
        //nome va assegnato dinamicamente in teoria
        emergency_types_ptr->emergency_desc = (char*)malloc(strlen((nome_temp1) + 1));
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
            fclose(file);
            return false;
        }
        strcpy(emergency_types_ptr->emergency_desc, nome_temp1);
        
        //finisco di caricare tutti i dati
        emergency_types_ptr->priority = prior_temp1;
        emergency_types_ptr->rescuers = request_arr;
        emergency_types_ptr->rescuer_required_number = req_count_vero;

        //faccio logging dei valori caricati
        sprintf(log_msg_buffer, "Caricata riga n* %d: Nome=%s, Priorità=%hd, Soccorsi richiesti=%d", riga_num_pass2, emergency_types_ptr->emergency_desc, emergency_types_ptr->priority, emergency_types_ptr->rescuer_required_number);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

        //stampo anche ogni Tipo di soccorso richiesto
        for(int i=0; i<emergency_types_ptr->rescuer_required_number; i++){
            sprintf(log_msg_buffer, "Soccorso=%s, Numero unità=%d, Tempo di gestine=%d", request_arr[i].type->rescuer_type_name, request_arr[i].required_count, request_arr[i].time_to_manage);
            log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        }
        indice_emergenza++;
        continue;
    }

    //ora posso dire quanti tipi di emergenza ho
    config->emergency_type_num = indice_emergenza;

    fclose(file);

    //come in rescuers se avevo almeno un candidato ma nessuno era valido considero fallimento
    if((config->emergency_type_num == 0) && (type_count > 0)){
        sprintf(log_msg_buffer, "Nessuna emergenza valida estratta da %d potenziali", type_count);
        log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
        
        //pulisco perchè l'array era stato allocato
        if(config->emergency_types_array){
            free(config->emergency_types_array);
        }
        config->emergency_types_array = NULL;
        return false;
    }
    //test emerg

    sprintf(log_msg_buffer, "Completato parsing. Letti %d emergenze valide", config->emergency_type_num);
    log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);
    return true;
}