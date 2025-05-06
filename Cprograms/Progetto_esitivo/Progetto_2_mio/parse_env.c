#include"parser.h"
#include"data_types.h"
#include"config1.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h> //torna un po' meglio per i found
#include<ctype.h>  //serve per isspace
#include<Syscalls_3_progetto.h>
#include<errno.h>
#include<logger.h>
#include <logger.c>

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
    
    //utilizzo una macro personalizzata che fa il lavoro di riconoscere l'errore, scriverlo sul buffer
    //fare log_message del buffer e ritornare false
    //per stampare più argomenti devo strettamente utilizzare una parentesi tonda
    //per delimitare gli argomenti di sprintf

    LOG_SNCALL(file, fopen(nome_file, "r"), (log_msg_buffer, "Fallimento apertura file '%s': %s", nome_file, strerror(errno)), LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

    log_message(LOG_EVENT_FILE_PARSING, nome_file, "File aperto con successo");

    int num_riga = 0;  //serve per file di logging
    char riga[LINE_LENGTH];
    bool coda_trovata = false;
    bool height_trovata = false;
    bool width_trovata = false;
    //utilizzo questi 3 booleani per fare da flag se sono stati trovati i dati

    env_config->queue_name[0] = "\0";
    env_config->grid_height = -1;
    env_config->grid_height = -1;
    //inizializzo tutto
    
    while(fgets(riga, sizeof(riga), file)){
        num_riga++;
        riga[strcspn(riga, "\n")] = 0;  //trova indice di \n, lo restituisce e ci scrive \0
    

        char* riga_temp = riga; //mi serve pk strtok modifica in place
        rimuovi_spazi(riga_temp);
        if(strlen(riga_temp == 0)){
            log_message(LOG_EVENT_FILE_PARSING, nome_file, "Riga %d vuota, ignoro", num_riga);
            continue;               //riga vuota non mi interessa
        }

        char* nome = strtok(riga_temp, "=");
        char* valore = strtok(NULL, "=");   //NULL vuol dire lavora sulla stessa stringa
        //divido dove trovo l'uguale come sintassi data dal prof ma possibili spazi
        //quindi faccio di nuovo rimuovi_spazi

        if(nome && valore){
            rimuovi_spazi(nome);
            rimuovi_spazi(valore);

            //a questo punto presuppongo di avere stringhe senza spazi ben formattate
            if(strcmp(nome, "queue") == 0){
                strncpy(env_config->queue_name, valore, sizeof(env_config->queue_name) -1);
                env_config->queue_name[sizeof(env_config->queue_name) - 1] = '\0';
                //metto sempre terminatore di stringa
                //uso sizeof(env_config->queue_name perchè la ho definita come dimensione [LINE_LENGTH])
                
                coda_trovata = true;
                sprintf(log_msg_buffer, "Da Riga %d ho estratto QUEUE = '%s'", num_riga, valore);
                log_message(LOG_EVENT_FILE_PARSING, nome_file, log_msg_buffer);

            }else if(strcmp(nome, "height") == 0){
                if((sscanf(valore, "%d", &env_config->grid_height) != 1) || (env_config->grid_height <=0)){
                    //il primo controllo guarda se è riuscito a convertire in intero 
                }
            }

        }
    }
}