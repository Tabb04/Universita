#define _POSIX_C_SOURCE 200809L 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mqueue.h>
#include <fcntl.h>    
#include <sys/stat.h> 
#include <time.h>     
#include <unistd.h>   
#include <errno.h>

#include "data_types.h" 
#include "config1.h"   
#include "Syscalls_3_progetto.h"

#define MQ_NOME "/emergenze674556"


//nel client non mi metto a loggare su file

int res;

bool invia_emergenze(mqd_t desc_coda, const char* nome, int x, int y, int delay){
    emergency_request_t emergenza;
    
    if(strlen(nome) >= EMERGENCY_NAME_LENGTH){  //mi fermo prima di mandare al server
        perror("Errore lunghezza");
        return EXIT_FAILURE;
    }
    strncpy(emergenza.emergency_name, nome, EMERGENCY_NAME_LENGTH - 1);
    emergenza.emergency_name[EMERGENCY_NAME_LENGTH - 1] = '\0';
    emergenza.x = x;
    emergenza.y = y;

    emergenza.timestamp = time(NULL);

    printf("Aspetto %d secondi prima di inviare l'emergenza\n", delay);
    sleep(delay);

    //non uso SCALL perchè devo ritornare false
    if(mq_send(desc_coda, (const char*)&emergenza, sizeof(emergenza), 0) == - 1){
        perror("Errore mq_send");
        return false;
    }
    return true;
}



int main(int argc, char* argv[]){

    mqd_t mq_desc;
    bool success = true;

    if(argc<2){ //argomenti minimo tre, ./client, -f, nome_file
        return EXIT_FAILURE;
    }

    SCALL_PERSONALIZED(mq_desc, mq_open(MQ_NOME, O_WRONLY), (mqd_t) -1, "Errore mq_open");
    

    //prima faccio modalità file
    if(strcmp(argv[1], "-f") == 0){
        //devo avere 3 argomenti
        if(argc != 3){
            SCALL(res, mq_close(mq_desc), "Errore mq_close");
           return EXIT_FAILURE;
        }
        FILE* file;
        SNCALL(file, fopen(argv[2], "r"), "Errore fopen");

        char buffer[LINE_LENGTH];   //uso stessa LINE_LENGTH
        int num_linea;
        while(fgets(buffer, sizeof(buffer), file)){
            num_linea++;
            buffer[strcspn(buffer, "\n")] = 0;

            char nome_da_file[EMERGENCY_NAME_LENGTH];
            int x_da_file, y_da_file, delay_da_file;

            //come separatori uso lo spazio
        
            if(sscanf(buffer, "%s %d %d %d", nome_da_file, &x_da_file, &y_da_file, &delay_da_file) == 4){
                if(!invia_emergenze(mq_desc, nome_da_file, x_da_file, y_da_file, delay_da_file)){
                    success = false;
                }else{
                    success = true;
                }
            }else{
                success = false;
            }
        }
        SCALL(res, fclose(file), "Errore fclose");
        
    }else{
        //da riga di comando
        if(argc != 5){
            SCALL(res, mq_close(mq_desc), "Errore mq_close");
            return EXIT_FAILURE;
        }
        const char* nome_emergenza = argv[1];
        int x_emergenza = atoi(argv[2]);
        int y_emergenza = atoi(argv[3]);
        int delay = atoi(argv[4]);
        
        if(!invia_emergenze(mq_desc, nome_emergenza, x_emergenza, y_emergenza, delay)){
            success = false;
        }else{
            success = true;
        }

    }
    
    SCALL(res, mq_close(mq_desc), "Errore mq_close");
    printf("Emergenza inviata\n");
    
    if(success == true){
        return EXIT_SUCCESS;
    }else{
        return EXIT_FAILURE;
    }

}