#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h> // Per future implementazioni multithread
#include <mqueue.h>  // Per code di messaggi POSIX
#include <fcntl.h>   // Per O_flags in mq_open
#include <sys/stat.h> // Per mode in mq_open
#include <errno.h>

#include "config1.h"
#include "data_types.h"
#include "logger.h"
#include "parser.h"
#include "Syscalls_3_progetto.h" 

#define NOME_LOGGER "emergency_system.log"
#define NUM_THREADS_GESTORI 3


//variabili globali
system_config_t global_system_config;
rescuer_digital_twin_t* global_digital_twins_array = NULL;
int total_digital_twins_creati = 0;
long next_emergency_id = 1; //Contatore id per le emergenze


//utilizzo una struttura per le emergenze che usa liste linkate
typedef struct nodo_emergenza{
    emergency_t data;
    struct nodo_emergenza *next;
}emergency_node_t;


//struttura per passaggio dei parametri torna meglio così
typedef struct{
    int id_log;     //id usato per logging
}thread_args_t;


//definisco dunque una testa e una coda
emergency_node_t* lista_emergenze_attesa_testa = NULL;
emergency_node_t* lista_emergenze_attesa_coda = NULL;

mtx_t mutex_emergenze_attesa;   //lista va protetta tramite mutex
cnd_t cond_nuova_emergenza;     //per segnalare se arriva una nuova emergenza

//per terminazione controllata uso un flag
volatile bool shutdown_flag = false;    //se true termino il thread

//descrittore coda
mqd_t mq_desc_globale = (mqd_t)-1;   //inizializzazione a valore non valido secondo


void pulizia_e_uscita(bool config_loaded, bool digital_twins_loaded, bool mq_loaded, bool sync_loaded, bool mtx_loaded){
    
    log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Inizio pulizia");
    
    if(mq_loaded && (mq_desc_globale != (mqd_t)-1)){
        mq_close(mq_desc_globale);
        log_message(LOG_EVENT_MESSAGE_QUEUE, "Cleanup", "Coda messaggi chiusa");
    }

    if(digital_twins_loaded && global_digital_twins_array){
        free(global_digital_twins_array);
        global_digital_twins_array = NULL;
        log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Deallocato array gemelli digitali");
    }

    if(config_loaded){
        free_system_config(&global_system_config);
    }

    if(sync_loaded){
        mtx_destroy(&mutex_emergenze_attesa);
        cnd_destroy(&cond_nuova_emergenza);
        log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Distrutte mutexe cond_var");
    }

    if(mtx_loaded){
        mtx_destroy(&mutex_array_gemelli);
        log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Mutex per array dei gemelli digitali distrutto");
    }


    log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Pulizia compleatata");
    close_logger();
}


//adesso la funzione che il thread in ascolto utilizzerà
int message_queue_ascoltatore_func(){
    emergency_request_t richiesta_ricevuta;
    char log_msg_buffer[LINE_LENGTH + 250];
    unsigned int priority;  //verrà usato da mq recive

    log_message(LOG_EVENT_GENERAL_INFO, "Thread ascoltatore", "Thread ascoltatore avviato");

    while(!shutdown_flag){
        //attendo messaggio sulla coda
        ssize_t byte_letti = mq_receive(mq_desc_globale, (char*)&richiesta_ricevuta, sizeof(emergency_request_t), &priority);
        //uso tipatura a (char*) perchè il prototipo di mq_recive dice "char *msg_ptr" come secondo parametro
        
        if(shutdown_flag){  //controllo appena svegliato
            break;
        }

        if(byte_letti == -1){
            sprintf(log_msg_buffer, "Errore in mq_receive: %s. Il thread in ascolto termina", strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
            //potrei segnalare al thread errore fatale
            break;
        }
        
        if(byte_letti != sizeof(emergency_request_t)){
            sprintf(log_msg_buffer, "Dimensione messaggio errata: %zd byte invece di %zd bytes", byte_letti, sizeof(emergency_request_t));
            log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
            continue;
        }

        //dovrei aver controllato tutto
        sprintf(log_msg_buffer, "Ricevuta emergenza: Tipo=%s, Posizione=(%d, %d), Timestamp=%ld", richiesta_ricevuta.emergency_name, richiesta_ricevuta.x, richiesta_ricevuta.y, richiesta_ricevuta.timestamp);
        log_message(LOG_EVENT_MESSAGE_QUEUE, "Nuova richiesta", log_msg_buffer);
        

        //metto a confrontare il tipo di emergenza tramite nome con quelli che ho
        //RICoRDA emergency_desc sarebbe il nome
        emergency_type_t* tipo_trovato = NULL;
        for(int i = 0; i<global_system_config.emergency_type_num; i++){
            if(strcmp(global_system_config.emergency_types_array[i].emergency_desc, richiesta_ricevuta.emergency_name) == 0){
                tipo_trovato = &global_system_config.emergency_types_array[i];
                break;
            }
        }

        //controllo se il tipo di emergenza è tra quelli memorizzati
        if(!tipo_trovato){
            sprintf("Tipo di emergenza '%s' sconosciuto. Ignoro richiesta", richiesta_ricevuta.emergency_name);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Richiesta invalida", log_msg_buffer);
        }

        //faccio tutti i controlli del caso        
        if((richiesta_ricevuta.x >= global_system_config.config_env.grid_height) || (richiesta_ricevuta.x > 0) || (richiesta_ricevuta.y >= global_system_config.config_env.grid_width) || (richiesta_ricevuta.y > 0)){
            sprintf(log_msg_buffer, "Coordinate di emergenza fuori dalla griglia per '%s'", richiesta_ricevuta.emergency_name);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Richiesta invalida", log_msg_buffer);
            continue;
        }

        //ora posso creare il nodo dell'emergenza
        emergency_node_t* nodo_em = (emergency_node_t*)malloc(sizeof(emergency_node_t));
        if(!nodo_em){
            sprintf(log_msg_buffer, "Fallita allocazione memoria per emergenza: '%s'", strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
            continue;
        }

        //inizializzo nodo
        nodo_em->data.type = tipo_trovato;  //data è di tipo emergency_t
        nodo_em->data.status = WAITING;
        nodo_em->data.x = richiesta_ricevuta.x;
        nodo_em->data.y = richiesta_ricevuta.y;
        nodo_em->data.time = time(NULL);    //timestamp di ricezione
        nodo_em->data.resquer_cont = 0;     //per ora non assegno nulla
        nodo_em->data.rescuer_dt = NULL;    //stesso
        nodo_em->next = NULL;               //devo vedere come è la coda
        

        //aggiungo nodo alla coda (con mutex)
        mtx_lock(&mutex_emergenze_attesa);
        if(lista_emergenze_attesa_coda == NULL){    //la lista è vuota
            lista_emergenze_attesa_testa = nodo_em;
            lista_emergenze_attesa_coda = nodo_em;
        }else{
            lista_emergenze_attesa_coda->next = nodo_em;
            lista_emergenze_attesa_coda = nodo_em;
        }

        sprintf(log_msg_buffer, "Aggiunga nuova emergenza alla coda in WAITING. Tipo=%s, Posizione=(%d, %d)", nodo_em->data.type->emergency_desc, nodo_em->data.x, nodo_em->data.y);
        log_message(LOG_EVENT_EMERGENCY_STATUS, "Coda attesa", log_msg_buffer);
        
        mtx_unlock(&mutex_emergenze_attesa);
        //poi segnalo una nuova emergenza
        cnd_signal(&cond_nuova_emergenza);    
    }
    log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
    return 0;
}


mtx_t mutex_array_gemelli;

//funzione per i thread gestori di emergenze
int gestore_emergenze_fun(void* arg){
        char log_msg_buffer[LINE_LENGTH + 200];
    thread_args_t* thrd_args = (thread_args_t*)arg;

    int id_thrd_log = thrd_args->id_log;
    emergency_node_t* emergency_node_current = NULL;    //nodo della lista

    sprintf(log_msg_buffer, "Avvio gestore emergenze #%d", id_thrd_log);
    log_message(LOG_EVENT_GENERAL_INFO, "Thread gestore", log_msg_buffer);
    
    while(!shutdown_flag){
        mtx_lock(&mutex_emergenze_attesa);  //devo attendere per prelevare un emergenza

        while((lista_emergenze_attesa_testa == NULL) && !shutdown_flag){
            //in questo caso aspetto finche non arrivi un emergenza o il flag di shutdown
            cnd_wait(&cond_nuova_emergenza, &mutex_emergenze_attesa);
        }

        if(shutdown_flag){  //ricontrollo
            mtx_unlock(&mutex_emergenze_attesa);
            break;
        }

        //prendo emergenza dalla testa della lista
        emergency_node_current = lista_emergenze_attesa_testa;
        lista_emergenze_attesa_testa = lista_emergenze_attesa_testa->next;

        if(lista_emergenze_attesa_testa == NULL){
            lista_emergenze_attesa_coda = NULL;
        }

        //qui dopo che mi sono preso l'emergenza non ho bisogno che mi tenga il lock
        //posso continuare a operare e lasciare che un altro thread prenda il successivo
        mtx_unlock(&mutex_emergenze_attesa);

        long temp_id = (long)emergency_node_current->data.time;

        sprintf(log_msg_buffer, "Thread #%d, Inizio processamento emergenza: Tipo=%s, Posizione=(%d, %d), Tempo ricezione=%ld", id_thrd_log, emergency_node_current->data.x, emergency_node_current->data.y, (long)emergency_node_current->data.time);
        log_message(LOG_EVENT_EMERGENCY_STATUS, "Gestore", log_msg_buffer);
    

        sprintf(log_msg_buffer, "Thread #%ld: Placeholder - Assegnazione soccorritori per emergenza ID %ld...", id_thrd_log, temp_id);
        log_message(LOG_EVENT_ASSIGNMENT, "N/A", log_msg_buffer);
    
        if(emergency_node_current->data.rescuer_dt != NULL){
            free(emergency_node_current->data.rescuer_dt);
            emergency_node_current->data.rescuer_dt = NULL;
        }
        free(emergency_node_current);
        emergency_node_current = NULL;
    }
    
    sprintf(log_msg_buffer, "Thread Gestore Emergenze #%d in terminazione.", id_thrd_log);
    log_message(LOG_EVENT_GENERAL_INFO, "Thread gestore", log_msg_buffer);

    free(thrd_args);
    return 0;
}




int main(int argc, char* argv[]){
    char log_msg_buffer[LINE_LENGTH + 250];

    //inizializzo logger
    if(!init_logger(NOME_LOGGER)){
        return EXIT_FAILURE;    //non posso scrivere su file, duh
    }

    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Sistema di Emergenze avviato");

    //flag per tracciare lo stato di inizializzaione per pulizia (a false)
    bool config_fully_loaded = false;
    bool digital_twins_inizializzati = false;   
    bool message_queue_open = false;
    bool sync_inizializzate = false;
    bool digital_twins_mutex_inizializzata = false;


    global_system_config.rescuers_type_array = NULL;
    global_system_config.instances_per_rescuer_type = NULL;
    global_system_config.emergency_types_array = NULL;
    global_system_config.rescuer_type_num = 0;
    global_system_config.emergency_type_num = 0;
    global_system_config.total_digital_twin_da_creare = 0;

    //inizio con tutti i parsing

    //parsing configurazione
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizio configurazione ambiente da env.conf");
    if(!parse_environment("env.conf", &global_system_config.config_env)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di env.conf");
        pulizia_e_uscita(false, false, false, false, false);     //nessuno dei due caricato in teoria
        return EXIT_FAILURE;
    }

    //parsing soccorritori
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Inizio configurazione soccorritori da rescuers.env");
    if(!parse_rescuers("rescuers.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di rescuers.conf");
        pulizia_e_uscita(true, false, false, false, false);  //true perchè env.conf potrebbe essere potenzialmente ok
        return EXIT_FAILURE;
    }

    //parsing emergenze
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Inizio configurazione emergenze da emergency_types.env");
    if(!parse_rescuers("emergency_types.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di emergency_types.conf");
        pulizia_e_uscita(true, false, false, false, false);
        return EXIT_FAILURE;
    }

    //andato tutto bene
    config_fully_loaded = true;
    log_message(LOG_EVENT_GENERAL_INFO, "main", "Configurazione avvenuta con successo");


    //controllo configurazione basic
    
    //no gemelli digitali
    if((global_system_config.total_digital_twin_da_creare == 0) && (global_system_config.rescuer_type_num > 0)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Presenti tipi di soccorritori ma nessun gemello digitale");
        pulizia_e_uscita(config_fully_loaded, false, false, false, false);
        return EXIT_FAILURE;
    }

    //coordinate
    for(int i = 0; i<global_system_config.rescuer_type_num; i++){
        rescuer_type_t *resc = &global_system_config.rescuers_type_array[i];

        if((resc->x) < 0 || (resc->y) < 0 || (resc->x >= global_system_config.config_env.grid_width) || (resc->y >= global_system_config.config_env.grid_height)){
            sprintf(log_msg_buffer, "Coordinate base del soccorritore '%s' fuori dalla griglia", resc->rescuer_type_name);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, false, false, false);
            return EXIT_FAILURE;
        }
    }


    //ora inizializzo i gemelli digitali
    if(global_system_config.total_digital_twin_da_creare > 0){
        global_digital_twins_array = (rescuer_digital_twin_t*)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if(!global_digital_twins_array){
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: '%s'", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, false, false, false, false);
            return EXIT_FAILURE;
        }
        
        sprintf(log_msg_buffer, "Allocato spazio per %d gemelli digitali: '%s'", global_system_config.total_digital_twin_da_creare, strerror(errno));
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);

        int indice_gemello = 0;
        for(int i = 0; i<global_system_config.emergency_type_num; i++){
            rescuer_type_t* tipo_corrente = &global_system_config.emergency_types_array[i];
            int istanze_da_fare = global_system_config.instances_per_rescuer_type[i];   //array che mi ero fatto per quanti ad ogn tipo
            //sarebbe stato meglio mettelo dentro il tipo stesso ma prof non mi risponde

            for(int j = 0; j<istanze_da_fare; j++){
                rescuer_digital_twin_t* gemello = &global_digital_twins_array[indice_gemello];
                gemello->id = indice_gemello+1;  //faccio l'id in base all'indice
                gemello->rescuer = tipo_corrente;
                gemello->x = tipo_corrente->x;   //parte dalla base
                gemello->y = tipo_corrente->y;
                gemello->status = IDLE;  //guarda elenco stato pdf

                sprintf(log_msg_buffer, "Gemello digitale creato, ID=%d, Tipo=%s", gemello->id, gemello->rescuer->rescuer_type_name);
                log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
                indice_gemello++;
            }
        }

        //indice è quanti ne ho creati
        total_digital_twins_creati = indice_gemello;
        if(total_digital_twins_creati != global_system_config.total_digital_twin_da_creare){
            sprintf(log_msg_buffer, "Discrepanza tra gemelli da creare %d e gemelli creati %d", global_system_config.total_digital_twin_da_creare, total_digital_twins_creati);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, false);
            return EXIT_FAILURE;
        }

        sprintf(log_msg_buffer, "Creati correttamente %d gemelli", global_system_config.total_digital_twin_da_creare);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
        
    }else{
        log_message(LOG_EVENT_GENERAL_INFO, "Main", "Nessun gemello creato come descritto da file");
    }


    //inizializzo sistemi di sincronizzazione
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizializzazione sistemi di sincronizzazione");
    
    //allora qui dovrei proteggere la sem_init con una SCALL anche se abbastanza esagerato
    //perchè nelle SCALL a lezione avevo 3 parametri
    //mentre qui avrei valore di ritorno, funzione, il tipo di evento, l'id per logger
    //+ messaggio per il logger, argomento 1 per pulizia e uscito, argomento 2, argomento 3, argomento 4
    //con un totale di 9 argomenti abbastanza poco pratico
    //comunque faccio provo ad implementarlo
    //se riesco faccio un #ifdef MACRO_VERSION in cui utilizzo le due versioni differenti
    

    int thrd_ret;
    sprintf(log_msg_buffer, "Fallita inizializzazione del mutex per coda emergenze");
    LOG_SCALL_THRDC(thrd_ret, mtx_init(&mutex_emergenze_attesa, mtx_plain), LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer, config_fully_loaded, digital_twins_inizializzati, message_queue_open, false, false);
    //questa macro dunque ma tmx_init, guarda se non è thrd_success e nel caso stampa il log_msg_buffer, pulisce e fa EXIT_FAILURE

    //in questo punto dovrei fare la stessa cosa con cnd_init ma il problema è che non posso
    //chiamare la funzione pulizia_e_uscita per questa con sync_loaded = true
    //visto che proverebbe a distruggerere una cond var non inizializzata
    //dovrei fare un altra macro ma ritengo 9 parametri già troppi

    if(cnd_init(&cond_nuova_emergenza) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallita inizializzazione di cond var per coda di emergenze");
        mtx_destroy(&mutex_emergenze_attesa);
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, false, false);
        return EXIT_FAILURE;
    }
    sync_inizializzate = true;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Sistemi di sincronizzazione inizializzati");

    if(mtx_init(&mutex_array_gemelli, mtx_plain) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallita inizializzazione di mutex per gemelli digitali");
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, false);
        return EXIT_FAILURE;
    }
    digital_twins_mutex_inizializzata = true;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Mutex per array gemelli digitali inizializzata");



    //adesso posso inizializzare la coda messaggi
    struct mq_attr attributi;
    attributi.mq_flags = 0;
    attributi.mq_maxmsg = 10;   //poi eventualmente cambia
    attributi.mq_msgsize = sizeof(emergency_request_t);
    attributi.mq_curmsgs = 0;

    sprintf(log_msg_buffer, "Apertura coda messaggi '/%s' in corso", global_system_config.config_env.queue_name);
    log_message(LOG_EVENT_MESSAGE_QUEUE, "Message queue", log_msg_buffer);

    char nome_mq_queue[LINE_LENGTH + 2];    //uno per / uno per \0
    snprintf(nome_mq_queue, sizeof(nome_mq_queue), "/%s", global_system_config.config_env.queue_name);
    
    //anche qui potrei provare ad utilizzare una macro ma ho comportamenti diversi
    //in base a se la coda è già inizializzata
    mq_desc_globale = mq_open(nome_mq_queue, O_CREAT | O_RDONLY | O_EXCL, 0660, &attributi);
    if(mq_desc_globale = (mqd_t)-1){
        if(errno == EEXIST){    //coda esiste già
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Main", "Coda messaggi già esistente, apro");
            mq_desc_globale = mq_open(nome_mq_queue, O_RDONLY);
        }
        if(mq_desc_globale = (mqd_t)-1){    //ha fallito di nuovo
            sprintf(log_msg_buffer, "Fallimento apertura coda con nome %s: '%s'", nome_mq_queue, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, false, sync_inizializzate, digital_twins_mutex_inizializzata);
            return EXIT_FAILURE;
        }
    }
    message_queue_open = true;
    log_message(LOG_EVENT_MESSAGE_QUEUE, "Main", "Coda messaggi aperta");


    //adesso posso creare il thread ascoltatore della coda messaggi
    thrd_t id_thread_listener;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Creazione thread ascoltatore delal coda messaggi");
    if(thrd_create(&id_thread_listener, message_queue_ascoltatore_func, NULL) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Maim", "Fallimento creazione thread listener coda messaggi");
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata);
        return EXIT_FAILURE;
    }
    
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Thread ascoltatore creato");

    //ora inizializzo gestori delle emergenze
    thrd_t array_id_gestori[NUM_THREADS_GESTORI];
    thread_args_t* array_args_gestori[NUM_THREADS_GESTORI];

    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Creazione gestori emergenze");
    for(int i = 0; i<NUM_THREADS_GESTORI; i++){

        //alloco memoria per gli argomenti del thread
        array_args_gestori[i] = (thread_args_t*)malloc(sizeof(thread_args_t));
        if(!array_args_gestori[i]){
            sprintf(log_msg_buffer, "Fallita allocazione di memoria per thread gestore #%d", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);

            //pulizia
            shutdown_flag = true;
            cnd_broadcast(&cond_nuova_emergenza);

            //se sono qui l'ascoltatore era stato avviato quindi va terminato, controllo chiusura
            if((thrd_join(id_thread_listener, NULL)) != thrd_success){
                log_message(LOG_EVENT_GENERAL_ERROR, "Cleanup", "Errore join del thread ascotatore");
            }else{
                log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Effettuata join thread ascoltatore");
            }

            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata);
            return EXIT_FAILURE;
        }
        array_args_gestori[i]->id_log = i;  //assegno l'id

        if(thrd_create(&array_id_gestori[i], gestore_emergenze_fun, array_args_gestori[i]) != thrd_success){
            sprintf(log_msg_buffer, "Errore creazione thread gestore #%d", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);

            shutdown_flag = true;
            cnd_broadcast(&cond_nuova_emergenza);

            //termino thread ascoltatore
            if((thrd_join(id_thread_listener, NULL)) != thrd_success){
                log_message(LOG_EVENT_GENERAL_ERROR, "Cleanup", "Errore join del thread ascotatore");
            }else{
                log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Effettuata join thread ascoltatore");
            }

            //devo joinare tutti quelli creati fino a quello fallito
            for(int j = 0; j<i; j++){
                thrd_join(array_id_gestori[j], NULL);
                if((thrd_join(array_id_gestori[i], NULL)) != thrd_success){
                    sprintf(log_msg_buffer, "Errore join del thread gestore #%d", j);
                    log_message(LOG_EVENT_GENERAL_ERROR, "Cleanup", log_msg_buffer);
                }else{
                    sprintf(log_msg_buffer, "Effettuata join per thread gestore #%d", j);
                    log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", log_msg_buffer);
                }
                free(array_args_gestori[j]);
            }
            free(array_args_gestori[i]);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata);
            return EXIT_FAILURE;
        }
        sprintf(log_msg_buffer, "Thread gestore #%d avviato", i);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
    }
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Tutti i thread gestori avviati");
    

    
    //chiusura gestori
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Invio shutdown ai tread");
    shutdown_flag = true;
    cnd_broadcast(&cond_nuova_emergenza);   //sveglio i gestori

    if(message_queue_open && (mq_desc_globale != (mqd_t)-1)){
        //SCRIVI
    }


    //aspetto terminazione del thread ascoltatore e controllo valore di uscita
    int res_listener;   //intero
    //anche qui potrei proteggere con macro
    if(thrd_join(id_thread_listener, &res_listener) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallimento di join thread ascoltatore");
    }else{
        sprintf(log_msg_buffer, "Thread ascoltatore terminato: '%d'", res_listener);
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
    }

    for(int i = 0; i<NUM_THREADS_GESTORI; i++){
        if(thrd_join(array_id_gestori[i], res_listener) != thrd_success){
            sprintf(log_message, "Fallimento di join thread su gestore #%d", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
        }else{
            sprintf(log_message, "Thread gestore #%d terminato", i);
            log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
        }
    }
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Tutti i threadsono stati terminati");

    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Pulizia risorse");

    pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata);
    //il logger viene chiuso in cleanup
    printf("Sistema terminato correttamente\n");
    return EXIT_SUCCESS;
}

