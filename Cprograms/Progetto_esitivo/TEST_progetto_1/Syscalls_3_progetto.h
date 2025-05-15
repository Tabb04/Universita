#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdio.h>
#include <stdlib.h>

#define SCALL_ERROR -1

#define SCALL(r, c, e) do { if((r = c) == SCALL_ERROR) { perror(e); exit(EXIT_FAILURE); } } while(0)

#define SNCALL(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)

#define SCALL_PERSONALIZED(r, c, err, e) do {if((r = c ) == err) { perror(e); exit(EXIT_FAILURE); } } while(0)
//per sem_open = SEM_FAILED, per mmap = MAP_FAILED

#define SCALL_MQREAD(r, c, buff, err) \
    do{ \
        if((r = c) == -1){ \
            perror(err); \
            exit(EXIT_FAILURE); \
        }else{ \
            printf("Messaggio: %s\n", buff); \
        } \
    }while(0)


#define SCALLREAD(a, b, c, d) \
    do{ \
        while((a = b) > 0){ \
            c; \
        }\
        if (a == -1){ \
            perror(d);\
            exit(EXIT_FAILURE);\
        }\
    }while(0)



#define PARENT_OR_CHILD(pid,f_parent,f_child)\
    do {\
        if(pid == 0){\
            f_child();\
        } else {\
            f_parent(pid);\
        }\
    }while(0)\






//SCALL UTILIZZATE PER PROGETTO ESTIVO


#define E_SCALL(val, fun, err)\
    do{\
        if((val = fun) == -1){\
            perror(err);\
            return false;\
        }\
    }while(0)\
//non faccio exit perchè voglio gestirlo fuori l'uscita
#define E_SNCALL(val, fun, err)\
    do{\
        if((val = fun) == NULL){\
            perror(err);\
            return false;\
        }\
    }while(0)\


//syscalls miglirote con scrittura su file di logging
//non faccio exit perchè faccio gestire al chiamante

/*
    if (!file) {
        sprintf(log_msg_buffer, "Fallimento apertura file '%s': %s", filename, strerror(errno));
        log_message(LOG_EVENT_FILE_PARSING, filename, log_msg_buffer);
        // perror("Errore apertura file environment"); // Sostituito da log
        // fprintf(stderr, "Impossibile aprire il file di configurazione ambiente: %s\n", filename); // Sostituito da log
        return false;
    }
*/

//Non so come fare a passare alla macro più parametri
//tipo sopra dove ho "Fallimento apertura file %s: %s, filename, sterror(errno)"
//se utilizzo macro al momento potrei solo utilizzare un parametro
#define LOG_SCALL1(val, fun, sprintf_args, event_type, id, msg_or_buf)\
    do{\
        if((val = fun) == -1){\
            sprintf sprintf_args;\
            log_message(event_type, id, msg_or_buf);\
            return false;\
        }\
    }while(0)\

#define LOG_SNCALL1(val, fun, sprintf_args, event_type, id, msg_or_buf)\
    do{\
        if((val = fun) == NULL){\
            sprintf sprintf_args;\
            log_message(event_type, id, msg_or_buf);\
            return false;\
        }\
    }while(0)\



//prima passavo i parametri con cui fare sprintf ma giustamente posso prima scrivere nel
//log_msg_buffer e poi eventualmente stampare 
//qui migliore 

#define LOG_SCALL(val, fun, event_type, id, formatted_msg_buffer) \
    do{\
        if((val = fun) == -1){\
            log_message(event_type, id, formatted_msg_buffer);\
            return false;\
        }\
    } while(0)

#define LOG_SNCALL(val, fun, event_type, id, formatted_msg_buffer)\
    do{\
        if((val = fun) == NULL){\
            log_message(event_type, id, formatted_msg_buffer);\
            return false;\
        }\
    }while(0)


    //non metto perror tanto è sul file log




    //macro per inizializzazione di mutex e cond var

    #define LOG_SCALL_THRDSC(val, fun, event_type, id, msg, cl_1, cl_2, cl_3, cl_4, cl_5, cl_6)\
    do{\
        if((val = (fun)) != thrd_success){\
            log_message(event_type, id, msg);\
            pulizia_e_uscita(cl_1, cl_2, cl_3, cl_4, cl_5, cl_6);\
            return EXIT_FAILURE;\
        }\
    }while(0)


#endif