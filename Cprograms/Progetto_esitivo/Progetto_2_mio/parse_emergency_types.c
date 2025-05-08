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
    rewind(file);
    int riga_num_pass2 = 0;
    int indice_emergenza = 0;

    //qui effettiva estrazione e convalidazione dei valori
    log_message(LOG_EVENT_FILE_PARSING, nome_file, "Secondo passsaggio: Estraggo dati");

}