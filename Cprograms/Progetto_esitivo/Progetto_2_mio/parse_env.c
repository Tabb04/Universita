#include"parser.h"
#include"data_types.h"
#include"config1.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h> //torna un po' meglio per i found
#include<ctype.h>  //serve per isspace
#include<Syscalls_3_progetto.h>

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
    if(!nome_file || !env_config){
        perror("Puntatori nulli");
        return false;   //non faccio exit, ho bisogno di sapere se la funzione è andaa a buon fine
    }

    FILE* file;
    //qui non uso le SCALL usate a lezione ma uso un gestore SCALL che non fa exit perchè
    //voglio gestire l'errore dal chiamante
    E_SNCALL(file, fopen(nome_file, "r"), "Errore fopen");

    int num_linea = 0;  //serve per file di logging
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
        riga[strcspn(riga, "\n")] = 0;  //trova indice di \n, lo restituisce e ci scrive \0
    

        char* riga_temp = riga; //mi serve pk strtok modifica in place
        rimuovi_spazi(riga_temp);
        if(strlen(riga_temp == 0)){
            continue;               //riga vuota non mi interessa
        }

        char* nome = strtok(riga_temp, "=");
        char* valore = strtok(NULL, "=");   //NULL vuol dire lavora sulla stessa stringa
        //divido dove trovo l'uguale come sintassi data dal prof ma possibili spazi

        if(nome && valore){
            rimuovi_spazi(nome);
            rimuovi_spazi(valore);

            //a questo punto presuppongo di avere stringhe senza spazi ben formattate
            if(strcmp(nome, "queue") == 0){
                strncpy(env_config->queue_name, valore, sizeof(env_config->queue_name) -1);
                //uso sizeof(env_config->queue_name perchè la ho definita come dimensione [LINE_LENGTH])
                coda_trovata = true;
            }else if(strcmp(nome, "height") == 0){
                if(sscanf(valore, "%d", &env_config->grid_height) != 1 || env_config->grid_height <=0){

                }
            }

        }
    }
}