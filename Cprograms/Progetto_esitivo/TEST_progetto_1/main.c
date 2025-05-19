#define _POSIX_C_SOURCE 200809L // Enable POSIX features for sigaction
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h> 
#include <mqueue.h>  
#include <fcntl.h>   //per O_flags in mq_open
#include <sys/stat.h> //per mode in mq_open
#include <errno.h>
#include <math.h>   //uso ciel e abs
#include <signal.h> //per gestione segnali
#include <unistd.h> //per write nella gestione segnali

#include "config1.h"
#include "data_types.h"
#include "logger.h"
#include "parser.h"
#include "Syscalls_3_progetto.h" 

#define NOME_LOGGER "emergency_system.log"
#define NUM_THREADS_GESTORI 3


//variabili globali
system_config_t global_system_config;
rescuer_digital_twin_t* global_gemelli_array = NULL;
int total_digital_twins_creati = 0;
long next_emergency_id = 1; //Contatore id per le emergenze


//utilizzo una struttura per le emergenze che usa liste linkate
typedef struct nodo_emergenza{
    emergency_t data;
    char id_generato[64];
    struct nodo_emergenza *next;
}emergency_node_t;

//variabili per la generazione dell'id
time_t ultimo_timestamp = 0;
int    counter_id_gen = 0;
mtx_t  mutex_generatore_id; //mutex per proteggere l'accesso alle variabili sopra



//struttura per passaggio dei parametri torna meglio così
typedef struct{
    int id_log;     //id usato per logging
}thread_args_t;


//definisco dunque una testa e una coda
emergency_node_t* lista_emergenze_attesa_testa = NULL;
emergency_node_t* lista_emergenze_attesa_coda = NULL;

mtx_t mutex_emergenze_attesa;   //lista va protetta tramite mutex
cnd_t cond_nuova_emergenza;     //per segnalare se arriva una nuova emergenza
mtx_t mutex_array_gemelli;

//per terminazione controllata uso un flag
volatile bool shutdown_flag = false;    //se true termino il thread

//descrittore coda
mqd_t mq_desc_globale = (mqd_t)-1;   //inizializzazione a valore non valido secondo

void genera_id_func(char *buff, size_t buff_len) {
    mtx_lock(&mutex_generatore_id);

    time_t current_time = time(NULL);
    if (current_time == ultimo_timestamp) {
        counter_id_gen++;
    } else {
        ultimo_timestamp = current_time;
        counter_id_gen = 0; // Resetta il contatore per il nuovo secondo
    }
    //copia il valore del contatore prima di sbloccare il mutex
    //nel caso un altro thread modifichi counter_id_gen subito dopo
    int id_suffix = counter_id_gen;

    mtx_unlock(&mutex_generatore_id);

    //formato ID: EMG_timestamp_secondi_contatore_nel_secondo
    snprintf(buff, buff_len, "EMG_%ld_%d", (long)current_time, id_suffix);
}



static void gestore_segnali(int sig){
    (void)sig;
    const char* msg = "Ctrl+c rilevato, chiusura\n"; 
    
    (void)write(STDERR_FILENO, msg, strlen(msg));   //per togliere warning
    shutdown_flag = true;
}


void pulizia_e_uscita(bool config_loaded, bool digital_twins_loaded, bool mq_loaded, bool sync_loaded, bool mtx_loaded, bool id_mtx_loaded){
    
    log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Inizio pulizia");
    
    if(mq_loaded){
        mq_close(mq_desc_globale);
        log_message(LOG_EVENT_MESSAGE_QUEUE, "Cleanup", "Coda messaggi chiusa");
    }

    if(digital_twins_loaded){   //&& global_gemelli_array a volte non va
        free(global_gemelli_array);
        global_gemelli_array = NULL;
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

    if(id_mtx_loaded){
        mtx_destroy(&mutex_generatore_id);
        log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Mutex per generatore ID distrutto");
    }

    log_message(LOG_EVENT_GENERAL_INFO, "Cleanup", "Pulizia compleatata");
    close_logger();
}


//adesso la funzione che il thread in ascolto utilizzerà
int message_queue_ascoltatore_func(){
    emergency_request_t richiesta_ricevuta;
    char log_msg_buffer[LINE_LENGTH + 250];
    unsigned int priority;  //verrà usato da mq recive
    ssize_t byte_letti;
    
    //allora qui voglio implementare la gestione segnali e per fare ciò
    //utilizzo mq_timedrecive invece di mq_recive
    //questo perchè metto un timer sulla lettura della coda e quando scade
    //ricontrollo se la shutdown flag è stata settata
    //se no continuo
    struct timespec mq_timeout;


    log_message(LOG_EVENT_GENERAL_INFO, "Thread ascoltatore", "Thread ascoltatore avviato");

    while(!shutdown_flag){

        if(clock_gettime(CLOCK_REALTIME, &mq_timeout) == -1){   //serve per impostare il timeout, se fallisce -1
            sprintf(log_msg_buffer, "Errore in clock_gettime: %s. Thread listener terminerà.", strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
            break; // Errore grave
        }
        mq_timeout.tv_sec += 1; //aggiungo un secondo al tempo corrente

        byte_letti = mq_timedreceive(mq_desc_globale, (char*)&richiesta_ricevuta, sizeof(emergency_request_t), &priority, &mq_timeout);


        if(shutdown_flag){
            log_message(LOG_EVENT_GENERAL_INFO, "Thread ascoltatore", "Shutdown flag rilevato dopo mq_timedreceive quindi termino");
            break;
        }


        if (byte_letti == -1) {
            if (errno == EINTR) { //interrotto da segnale
                //handler dovrebbe aver impostato shutdown_flag

                log_message(LOG_EVENT_MESSAGE_QUEUE, "Thread ascoltatore", "mq_timedreceive interrotto da segnale, guardo shutdown_flag");
                if(shutdown_flag){
                    break;
                }
                continue; //magari un atlro segnale non gestito
            }else if(errno == ETIMEDOUT){
                //vuol dire è scaduto il timer, continuo
                continue;
            }else{
                sprintf(log_msg_buffer, "Errore in mq_timedreceive: %s. Termino Thread Ascoltatore", strerror(errno));
                log_message(LOG_EVENT_GENERAL_ERROR, "Thread ascoltatore", log_msg_buffer);
                break;
            }
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
            sprintf(log_msg_buffer, "Tipo di emergenza '%s' sconosciuto. Ignoro richiesta", richiesta_ricevuta.emergency_name);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Richiesta invalida", log_msg_buffer);
            continue;
        }

        //faccio tutti i controlli del caso        
        if((richiesta_ricevuta.x >= global_system_config.config_env.grid_width) || (richiesta_ricevuta.x < 0) || (richiesta_ricevuta.y >= global_system_config.config_env.grid_height) || (richiesta_ricevuta.y < 0)){
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
        
        genera_id_func(nodo_em->id_generato, sizeof(nodo_em->id_generato));

        //aggiungo nodo alla coda (con mutex)
        mtx_lock(&mutex_emergenze_attesa);
        if(lista_emergenze_attesa_coda == NULL){    //la lista è vuota
            lista_emergenze_attesa_testa = nodo_em;
            lista_emergenze_attesa_coda = nodo_em;
        }else{
            lista_emergenze_attesa_coda->next = nodo_em;
            lista_emergenze_attesa_coda = nodo_em;
        }

        sprintf(log_msg_buffer, "Aggiunga nuova emergenza alla coda in WAITING. Tipo=%s, Posizione=(%d, %d), ID=%s", nodo_em->data.type->emergency_desc, nodo_em->data.x, nodo_em->data.y, nodo_em->id_generato);
        log_message(LOG_EVENT_EMERGENCY_STATUS, "Coda attesa", log_msg_buffer);
        
        mtx_unlock(&mutex_emergenze_attesa);
        //poi segnalo una nuova emergenza
        cnd_signal(&cond_nuova_emergenza);    
    }

    log_message(LOG_EVENT_GENERAL_INFO, "Thread ascoltatore", "Thread ascoltatore in terminazione");
    return 0; //per thread join

}

typedef struct {
    rescuer_digital_twin_t *dt;
    double time_to_arrive;
} rescuer_candidate_t;  //usata in gestore emergenze per gestire i candidati


//funzione per qsort dei candidati
static int sort_candidati(const void* a, const void* b){

    rescuer_candidate_t* primo = (rescuer_candidate_t*)a;
    rescuer_candidate_t* secondo = (rescuer_candidate_t*)b;

    if(primo->time_to_arrive < secondo->time_to_arrive){
        return -1;
    }
    if(primo->time_to_arrive > secondo->time_to_arrive){
        return 1;
    }
    return 0;
}






//funzione per i thread gestori di emergenze
int gestore_emergenze_fun(void* arg){
    thread_args_t* thread_args = (thread_args_t*)arg;

    if(!thread_args){
        log_message(LOG_EVENT_GENERAL_ERROR, "Thread Gestore", "Argomenti del thread nulli");
        return -1; //guarda cosa mettere
    }

    int thread_id_log = thread_args->id_log;
    char log_msg_buffer[LINE_LENGTH + 200];     //qui non è detto sia definito
    emergency_node_t* current_nodo_emergency = NULL;
    
    sprintf(log_msg_buffer, "Thread Gestore #%d avviato.", thread_id_log);
    log_message(LOG_EVENT_GENERAL_INFO, "Thread Gestore", log_msg_buffer);

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

        //prendo emergenza dalla testa della lista e sposto la testa a quello dopo
        current_nodo_emergency = lista_emergenze_attesa_testa;
        lista_emergenze_attesa_testa = lista_emergenze_attesa_testa->next;

        if(lista_emergenze_attesa_testa == NULL){   //se si è svuotato 
            lista_emergenze_attesa_coda = NULL;
        }

        //qui dopo che mi sono preso l'emergenza non ho bisogno che mi tenga il lock
        //posso continuare a operare e lasciare che un altro thread prenda il successivo
        mtx_unlock(&mutex_emergenze_attesa);

        emergency_type_t* em_type = current_nodo_emergency->data.type;   //puntatore al tipo dell'emergenza estratta
       
        /*
        long emergency_id_log = (long)current_nodo_emergency->data.time;
        char emergency_id_string[50];   //facilito logging id
        snprintf(emergency_id_string, sizeof(emergency_id_string), "Emergenza %ld", emergency_id_log);
        */
        char *id_emergenza_log = current_nodo_emergency->id_generato;
        
        //---------------

        sprintf(log_msg_buffer, "Thread #%d, Inizio processamento emergenza %s: Tipo=%s, Posizione=(%d, %d), Tempo ricezione=%ld", thread_id_log, id_emergenza_log, em_type->emergency_desc, current_nodo_emergency->data.x, current_nodo_emergency->data.y, (long)current_nodo_emergency->data.time);
        log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, log_msg_buffer);
    
        //calcolo deadline
        time_t deadline = 0;
        if(em_type->priority == MEDIA_PRIOR){
            deadline = current_nodo_emergency->data.time + MEDIA_PRIOR_TIME;
        }else if(em_type->priority == ALTA_PRIOR){
            deadline = current_nodo_emergency->data.time + ALTA_PRIOR_TIME;
        }

        if(deadline > 0){
            sprintf(log_msg_buffer, "Thread #%d, calcolato deadline per emergenza %s: %ld secondi", thread_id_log, id_emergenza_log, (long)deadline);
            //long perchè lo è data.time essendo time_t
            log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
        }

        //struttura per selezione soccorritori
        int rescuers_necessari = 0;
        for(int i = 0; i<em_type->rescuer_required_number; i++){
            rescuers_necessari += em_type->rescuers[i].required_count;
        }

        //in questo array memorizzo i puntatori ai migliori candidati trovati
        rescuer_digital_twin_t** rescuers_selezionati = NULL;

        if(rescuers_necessari > 0){
            rescuers_selezionati = (rescuer_digital_twin_t**)malloc(rescuers_necessari * sizeof(rescuer_digital_twin_t *));
            if(!rescuers_selezionati){
                sprintf(log_msg_buffer, "Thread #%d, Fallimento allocazione memoria per soccorritori di emergenza %s", thread_id_log, id_emergenza_log);
                log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
                //libero e continuo
                current_nodo_emergency->data.status = CANCELED;
                log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza CANCELED");
                free(current_nodo_emergency);
           current_nodo_emergency = NULL;
                continue;
            }
        }else{
            sprintf(log_msg_buffer, "Thread #%d Nessun soccorritore richiesto per emergenza %s, possibile errore config", thread_id_log, id_emergenza_log);
            log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);
            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza CANCELED");
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        int current_idx = 0;
        bool possibile = true;
        double tempo_max_arrivo = 0.0;   //tempo max di arrivo tra tutti i soccorritori selezionati
        
        //ciclo per ogni soccorritore richiesto dall'emergenza
        for(int i = 0; i<em_type->rescuer_required_number; i++){
            if(!possibile){
                break;
            }
            rescuer_request_t* request = &em_type->rescuers[i];     //assegno primo tipo di soccorritore
            rescuer_type_t* tipo_richiesto = request->type;
            int required_count = request->required_count;
            
            rescuer_candidate_t* candidati = (rescuer_candidate_t*)malloc(total_digital_twins_creati * sizeof(rescuer_candidate_t));
            if(!candidati){
                sprintf(log_msg_buffer, "Thread #%d, Fallimento allocazione memoria per candidati soccorritori di emergenza %s", thread_id_log, id_emergenza_log);
                log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
                possibile = false;
                break;
            }
            int count_candidati = 0;
            
            //qui devo accedere allo stato dei soccorritori, mutex
            mtx_lock(&mutex_array_gemelli);
            for(int j = 0; j<total_digital_twins_creati; j++){
                rescuer_digital_twin_t* gemello = &global_gemelli_array[j];

                if((gemello->rescuer == tipo_richiesto) && (gemello->status == IDLE)){
                    //adesso calcolo il tempo di arrivo

                    int distanza = (abs(gemello->x - current_nodo_emergency->data.x) + abs(gemello->y - current_nodo_emergency->data.y));
                    double tempo_arrivo;
                    if(gemello->rescuer->speed > 0){
                        tempo_arrivo = ceil(((double)distanza) / (gemello->rescuer->speed));
                    }else{
                        tempo_arrivo = -1.0;
                    }
 
                    if(tempo_arrivo >= 0){
                        candidati[count_candidati].dt = gemello;
                        candidati[count_candidati].time_to_arrive = tempo_arrivo;
                        count_candidati++;
                    }else{
                        sprintf(log_msg_buffer, "Errore da Thread gestore #%d per emergenza %s. Soccorritore %s ha velocità <= 0", thread_id_log, id_emergenza_log, gemello->rescuer->rescuer_type_name);
                        log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);
                    }
                }
            }
            mtx_unlock(&mutex_array_gemelli);
            //sblocco l'array

            if(count_candidati < required_count){
                sprintf(log_msg_buffer, "Errore Thread #%d per emergenza %s. Risorse insufficienti, trovati %d candidati IDLE per tipo '%s', ma necessari %d", thread_id_log, id_emergenza_log, count_candidati, tipo_richiesto->rescuer_type_name, rescuers_necessari);
                log_message(LOG_EVENT_ASSIGNMENT, id_emergenza_log, log_msg_buffer);
                possibile = false;
                free(candidati);
                break;
            }

            //a questo punto ordino i candidati per tempo di arrivo con qsort con sort_candidati
            qsort(candidati, count_candidati, sizeof(rescuer_candidate_t), sort_candidati);

            //adesso che sono ordinato seleziono i primi required_count
            for(int k = 0; k<required_count; k++){
                if(current_idx < rescuers_necessari){
                    rescuers_selezionati[current_idx] = candidati[k].dt;    //assegno dall'array ordinato

                    //aggiorno tempo max arrivo
                    if(candidati[k].time_to_arrive > tempo_max_arrivo){
                        tempo_max_arrivo = candidati[k].time_to_arrive;     //cosi so il tempo massimo che arrivano tutti
                    }
                    current_idx++;
                }else{
                    //non ha senso in teoria, ricontrolla
                }
            }
            free(candidati);

            if(!possibile){//serve se c'è stato un errore dentro
                break;  
            }
        }
         //CONTROLLO DEADLINE
        if(possibile){
            time_t ora = time(NULL);
            time_t tempo_stimato_arrivo = ora + (time_t)tempo_max_arrivo;

            if((deadline > 0) && (tempo_stimato_arrivo > deadline)){
                sprintf(log_msg_buffer, "Errore Thread #%d per emergenza %s. TIMEOUT, tempo stimato supera deadline", thread_id_log, id_emergenza_log);
                log_message(LOG_EVENT_TIMEOUT, id_emergenza_log, log_msg_buffer);
                current_nodo_emergency->data.status =  TIMEOUT;
                log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a TIMEOUT, deadline non rispettato");
                
                //pulizia
                if(rescuers_selezionati){
                    free(rescuers_selezionati);
                    rescuers_selezionati = NULL;
                }
                free(current_nodo_emergency);
                current_nodo_emergency = NULL;
                continue; //prossima emergenza
            }

        }

        if(!possibile){
            if(rescuers_selezionati){
                free(rescuers_selezionati);
            }
            if(current_nodo_emergency->data.status != TIMEOUT){
                //nel caso fosse uscito per un altro motivo
                current_nodo_emergency->data.status = TIMEOUT;
                log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a TIMEOUT)");
            }
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        //ASSEGNAMENTO
        sprintf(log_msg_buffer, "Assegnamento effettuato, tempo previsto di arrivo %.0f secondi", tempo_max_arrivo);
        log_message(LOG_EVENT_ASSIGNMENT, id_emergenza_log, log_msg_buffer);

        //alloco array di puntatori nell'emergenza
        current_nodo_emergency->data.rescuer_dt = (rescuer_digital_twin_t **)malloc(rescuers_necessari * sizeof(rescuer_digital_twin_t *));
        if(!current_nodo_emergency->data.rescuer_dt){

            
            sprintf(log_msg_buffer, "Errore thread #%d, fallita malloc per assegnamento di soccorritori", thread_id_log);
            log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);

            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED");
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            if(rescuers_selezionati){
                free(rescuers_selezionati);
            }
            continue;   //passo alla prossima emergenza
        }


        //qui blocco critico perchè aggiorno stato di soccorritori globali
        mtx_lock(&mutex_array_gemelli);
        bool conflitto = false; //se hanno rubato un gemello

        for(int i = 0; i<rescuers_necessari; i++){
            rescuer_digital_twin_t* gemello_da_assegnare = rescuers_selezionati[i];

            if(gemello_da_assegnare->status != IDLE){

                //questo significa che un altro thread ha preso questo gemello nel frattempo
                sprintf(log_msg_buffer, "Errore thread #%d, soccorritore ID=%d non è più idle", thread_id_log, gemello_da_assegnare->id);
                log_message(LOG_EVENT_ASSIGNMENT, id_emergenza_log, log_msg_buffer);
                conflitto = true;

                for(int k = 0; k<i; k++){
                    current_nodo_emergency->data.rescuer_dt[k]->status = IDLE;
                    //rimetto a idle tutti i soccorritori prima che erano diventati EN_ROUTE_TO_SCENE
                }
                break;
            }

            //assegno puntatore del soccorritore globale all'array dell'emergenza
            current_nodo_emergency->data.rescuer_dt[i] = gemello_da_assegnare;
            gemello_da_assegnare->status = EN_ROUTE_TO_SCENE;


            char rescuer_id_string[30]; //stessa roba che con id_emergenza_log
            snprintf(rescuer_id_string, sizeof(rescuer_id_string), "Soccorritore %d", gemello_da_assegnare->id);
            sprintf(log_msg_buffer, "Soccorritore %d assegnato ad emergenza %s", gemello_da_assegnare->id, id_emergenza_log);
            log_message(LOG_EVENT_RESCUER_STATUS, rescuer_id_string, log_msg_buffer);
        }
        mtx_unlock(&mutex_array_gemelli);
        //rilascio mutex


        if(rescuers_selezionati){
            free(rescuers_selezionati);
            rescuers_selezionati = NULL;
        }

        if(conflitto){
            if(current_nodo_emergency->data.rescuer_dt != NULL){    //controllo per sicurezza
                free(current_nodo_emergency->data.rescuer_dt);
                current_nodo_emergency->data.rescuer_dt = NULL;
            }

            current_nodo_emergency->data.status = TIMEOUT;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Impostato stato emergenza TIMEOUT");
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        current_nodo_emergency->data.resquer_cont = rescuers_necessari;
        current_nodo_emergency->data.status = ASSIGNED;
        
        log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Impostato stato emergenza ASSIGNED");




        //GESTIONE EMERGENZA
        //per primo viaggio e arrivo all'emergenza

        sprintf(log_msg_buffer, "Thread #%d. Inizia viaggio dei soccorritori", thread_id_log);
        log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);

        //prima voglio calcolare i tempi di arrivo individuali
        double* tempi_di_arrivo_individuali = (double*)calloc(current_nodo_emergency->data.resquer_cont, sizeof(double));
        bool* sulla_scena = (bool*)calloc(current_nodo_emergency->data.resquer_cont, sizeof(bool));
        //uso calloc in questo caso così non ho valori spazzatura

        if(!tempi_di_arrivo_individuali || !sulla_scena){
            sprintf(log_msg_buffer, "Errore Thread #%d. Fallita allocazione per tempi di arrivo e/o bool su scena", thread_id_log);
            log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);

            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED");
            
            //aggiorno i gemelli prima e rimetto a idle
            mtx_lock(&mutex_array_gemelli);
            for(int k = 0; k < current_nodo_emergency->data.resquer_cont; k++) {
                if (current_nodo_emergency->data.rescuer_dt[k] != NULL) {
                    current_nodo_emergency->data.rescuer_dt[k]->status = IDLE;
                }
            }
            mtx_unlock(&mutex_array_gemelli);

            //non so quale abbia fallito
            if(tempi_di_arrivo_individuali){
                free(tempi_di_arrivo_individuali);
            }
            if(sulla_scena){
                free(sulla_scena);
            }
            if(current_nodo_emergency->data.rescuer_dt){
                free(current_nodo_emergency->data.rescuer_dt);
                current_nodo_emergency->data.rescuer_dt = NULL;
            }

            free(current_nodo_emergency);
            current_nodo_emergency = NULL; 
            continue;
        }

        //tempo per simulare
        //time_t inizio_simulazione = time(NULL);
        double tempo_arrivo_tot_max = 0.0;

        for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
            rescuer_digital_twin_t* gemello = current_nodo_emergency->data.rescuer_dt[i];
            int distanza = (abs(gemello->x - current_nodo_emergency->data.x)) + (abs(gemello->y - current_nodo_emergency->data.y));
            if(gemello->rescuer->speed > 0){
                tempi_di_arrivo_individuali[i] = ceil(((double)distanza) / (gemello->rescuer->speed));
            }else{
                tempi_di_arrivo_individuali[i] = -1.0;
            }
            sulla_scena[i] = false;
            
            if(tempi_di_arrivo_individuali[i] < 0){
                sprintf(log_msg_buffer, "Errore thread #%d per emergenza %s. Velocità invalida", thread_id_log, id_emergenza_log);
                log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);
                
                //non è possibile gestire l'emergenza
                possibile = false; //riuso sto flag
                break;
            }

            //devo salvarmi possibilmente il più lento
            if(tempi_di_arrivo_individuali[i] > tempo_arrivo_tot_max){
                tempo_arrivo_tot_max = tempi_di_arrivo_individuali[i];
            }
        }

        if(!possibile){
            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED");
            
            //faccio stessa cosa sui gemelli prima
            mtx_lock(&mutex_array_gemelli);
            for(int j = 0; j < current_nodo_emergency->data.resquer_cont; j++) {
                if(current_nodo_emergency->data.rescuer_dt[j] != NULL){
                    current_nodo_emergency->data.rescuer_dt[j]->status = IDLE;
                }
            }
            mtx_unlock(&mutex_array_gemelli);

            //libero tutto
            if(rescuers_selezionati){
                free(rescuers_selezionati);
            }
            free(tempi_di_arrivo_individuali);
            free(sulla_scena);
            if(current_nodo_emergency->data.rescuer_dt) {
                free(current_nodo_emergency->data.rescuer_dt);
            }
            current_nodo_emergency->data.rescuer_dt = NULL;
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }


        //a questo punto potrei semplicemente aspettare il tempo necessario
        //ma preferisco andare ad intervalli di un secondo all volta
        //per gestire il controllo della shutdown flag e gli arrivi scaglionati


        int rescuer_arrivati = 0;
        for(double i = 0; i<tempo_arrivo_tot_max; i += 1.0){    //si incrementa così il double
            if(shutdown_flag){
                sprintf(log_msg_buffer, "Thread #%d, Richiesto shutdown durante viaggio soccorritori", thread_id_log);
                log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
                possibile = false;
                break;
            }

            thrd_sleep(&(struct timespec){.tv_sec = 1, .tv_nsec = 0}, NULL);    //come si faceva a lezione
            
            //ora controllo se qualche soccorritore è per caso arrivato
            for(int j = 0; j<current_nodo_emergency->data.resquer_cont; j++){
                if(!sulla_scena[j] && (i + 1.0) >= tempi_di_arrivo_individuali[j]){
                    //se non c'è ma il suo tempo di arrivo è minore
                    //del tempo corrente + 1 allora è arrivato
        
                    sulla_scena[j] = true;
                    rescuer_arrivati++;

                    //mi salvo chi è arrivato
                    rescuer_digital_twin_t* gemello_arrivato = current_nodo_emergency->data.rescuer_dt[j];
                    
                    mtx_lock(&mutex_array_gemelli);
                    //ora devo modificarlo cambiando stato e posizione
                    gemello_arrivato->status = ON_SCENE;
                    gemello_arrivato->x = current_nodo_emergency->data.x;
                    gemello_arrivato->y = current_nodo_emergency->data.y;
                    mtx_unlock(&mutex_array_gemelli);

                    char buf_soccorritore[30];  //loggare
                    snprintf(buf_soccorritore, sizeof(buf_soccorritore), "Soccorritore %d", gemello_arrivato->id);
                    sprintf(log_msg_buffer, "Soccorritore con ID=%d '%s', Arrivato ad emergenza %s. Aggiorata posizione e stato", gemello_arrivato->id, gemello_arrivato->rescuer->rescuer_type_name, id_emergenza_log);
                    log_message(LOG_EVENT_RESCUER_STATUS, buf_soccorritore, log_msg_buffer);
                }
            }

            //se tutti i soccorritori sono arrivati posso uscire dal loop di attesa
            if(rescuer_arrivati == current_nodo_emergency->data.resquer_cont){
                break;  
            }
        }
        free(tempi_di_arrivo_individuali);

        if(!possibile || shutdown_flag){
            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED");

            //pulisco i soccorritori EN_ROUTE o ON_SCENE

            mtx_lock(&mutex_array_gemelli);
            for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
                if(current_nodo_emergency->data.rescuer_dt[i] != NULL){
                    current_nodo_emergency->data.rescuer_dt[i]->status = IDLE;
                }
            }
            mtx_unlock(&mutex_array_gemelli);

            free(sulla_scena);
            if(current_nodo_emergency->data.rescuer_dt){
                free(current_nodo_emergency->data.rescuer_dt);
            }
            current_nodo_emergency->data.rescuer_dt = NULL;
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        //se sono qui tutti i soccorritori sono ON_SCENE
        //controllo comunque
        if (rescuer_arrivati < current_nodo_emergency->data.resquer_cont) {
            sprintf(log_msg_buffer, "Errore thread #%d, non sono arrivati tutti i soccorritori", thread_id_log);
            log_message(LOG_EVENT_GENERAL_ERROR, id_emergenza_log, log_msg_buffer);
            
            //pulisco anche qui
            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED (errore logico arrivi).");
            
            //aggiorno tutti i gemello
            mtx_lock(&mutex_array_gemelli);
            for(int j = 0; j < current_nodo_emergency->data.resquer_cont; j++){
                if(current_nodo_emergency->data.rescuer_dt[j] != NULL){
                    current_nodo_emergency->data.rescuer_dt[j]->status = IDLE;
                }
            }
            mtx_unlock(&mutex_array_gemelli);


            free(sulla_scena);
            if(current_nodo_emergency->data.rescuer_dt){
                free(current_nodo_emergency->data.rescuer_dt);
            }
            current_nodo_emergency->data.rescuer_dt = NULL;
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        //ora emergenza diventa IN_PROGRESS
        current_nodo_emergency->data.status = IN_PROGRESS;
        sprintf(log_msg_buffer, "Sono arrivati tutti i soccorritori per emergenza %s", id_emergenza_log);
        log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, log_msg_buffer);



        //adesso possiamo simulare il lavoro sulla scena
        double tempo_su_emergenza_max = 0.0;    //prendo il maggiore dei tempi
        for(int i = 0; i<current_nodo_emergency->data.type->rescuer_required_number; i++){
            //sto iterando sul numero di tipi richiesti non sui soccorritori che ho
            if(((double)current_nodo_emergency->data.type->rescuers[i].time_to_manage) > tempo_su_emergenza_max){
                tempo_su_emergenza_max = (double)current_nodo_emergency->data.type->rescuers[i].time_to_manage;
            }
        }

        sprintf(log_msg_buffer, "Inizio gestione emergenza '%s' da thread #%d sul posto, tempo stimato %.0f", id_emergenza_log, thread_id_log, tempo_su_emergenza_max);
        log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);


        //simulo tempo di gestione emergenza sul posto, controllando shutdown flag
        //faccio nello stesso modo incrementando di un secondo alla volta
        for(double i = 0.0; i<tempo_su_emergenza_max; i+=1){
            if(shutdown_flag){
                sprintf(log_msg_buffer, "Thread #%d, Richiesto shutdown durante gestione sul posto", thread_id_log);
                log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
                possibile = false;  //la uso per dire che non proseguo
                break;
            }
            thrd_sleep(&(struct timespec){.tv_sec = 1, .tv_nsec = 0}, NULL);
        }

        if(!possibile || shutdown_flag){
            current_nodo_emergency->data.status = CANCELED;
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a CANCELED");

            //sempre stessa cosa sistemo tutti i soccorritori
            mtx_lock(&mutex_array_gemelli);
            for(int j = 0; j<current_nodo_emergency->data.resquer_cont; j++){
                if(current_nodo_emergency->data.rescuer_dt[j] != NULL){
                    current_nodo_emergency->data.rescuer_dt[j]->status = IDLE;
                }
            }
            mtx_unlock(&mutex_array_gemelli);

            free(sulla_scena);
            if (current_nodo_emergency->data.rescuer_dt) free(current_nodo_emergency->data.rescuer_dt);
            current_nodo_emergency->data.rescuer_dt = NULL;
            free(current_nodo_emergency);
            current_nodo_emergency = NULL;
            continue;
        }

        //se sono qui vuol dire che ho finito di aspettare
        current_nodo_emergency->data.status = COMPLETED;
        log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Stato emergenza impostato a COMPLETED");

        
        //posso ritornare alla base, cambio stato dei gemelli
        mtx_lock(&mutex_array_gemelli);
        for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
            rescuer_digital_twin_t* gemello = current_nodo_emergency->data.rescuer_dt[i];   //per praticità
            gemello->status = RETURNING_TO_BASE;

            char buf_rescuer[30];   //per comodità
            snprintf(buf_rescuer, sizeof(buf_rescuer), "Soccorritore %d", gemello->id);
            sprintf(log_msg_buffer, "Soccorritore %d di tipo '%s' per emergenza %s ha completato il lavoro", gemello->id, gemello->rescuer->rescuer_type_name, id_emergenza_log);
            log_message(LOG_EVENT_RESCUER_STATUS, buf_rescuer, log_msg_buffer);
        }
        mtx_unlock(&mutex_array_gemelli);


        //devo anche simulare il ritorno verso la base

        sprintf(log_msg_buffer, "Thread #%d, emergenza %s inizia simulazione viaggio verso base",thread_id_log, id_emergenza_log);
        log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);

        double tempo_ritorno_max = 0.0;
        //riciclo sulla_scena per dire se è in base
        for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
            sulla_scena[i] = false;
        }

        for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){

            //è tutto identico tranne calcolato da emergenza/posizione del soccorritore a base
            rescuer_digital_twin_t* gemello = current_nodo_emergency->data.rescuer_dt[i];
            int distanza = (abs(gemello->x - gemello->rescuer->x)) + (abs(gemello->y - gemello->rescuer->y));
            double tempo;

            if(gemello->rescuer->speed > 0){
                tempo = ceil((double)distanza / gemello->rescuer->speed);
            }else{
                //in teoria non avrebbe senso essere qui, per sicurezza se ci arrivo do tempo istantaneo
                tempo = 0;
            }
            if(tempo_ritorno_max < tempo){
                tempo_ritorno_max = tempo;
            }
        }

        int soccorritore_tornati_base = 0;
        for(double j = 0; j<tempo_ritorno_max; j+=1){
            if(shutdown_flag){
                sprintf(log_msg_buffer, "Thread #%d, Richiesto shutdown durante ritorno in base", thread_id_log);
                log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);
                possibile = false;  //la uso per dire che non proseguo
                break;
            }
            thrd_sleep(&(struct timespec){.tv_sec = 1, .tv_nsec = 0}, NULL);

            for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
                if(!sulla_scena[i]){
                    rescuer_digital_twin_t* gemello_rientrante = current_nodo_emergency->data.rescuer_dt[i];
                    int distanza = abs(gemello_rientrante->x - gemello_rientrante->rescuer->x) + abs(gemello_rientrante->y - gemello_rientrante->rescuer->y);
                    double tempo_rientro;
                    if(gemello_rientrante->rescuer->speed > 0){
                        tempo_rientro = ceil((double)distanza / gemello_rientrante->rescuer->speed);
                    }else{
                        //stessa cosa qui non dovrebbe essere possibile ma per sicurezza faccio istantaneo
                        tempo_rientro = 0;
                    }

                    if(tempo_rientro < (j+1.0)){    //stesso controllo dell'andata
                        sulla_scena[i] = true;
                        soccorritore_tornati_base++;

                        //metto il gemello di nuovo in base e lo metto idle
                        mtx_lock(&mutex_array_gemelli);
                        gemello_rientrante->status = IDLE;
                        gemello_rientrante->x = gemello_rientrante->rescuer->x;
                        gemello_rientrante->y = gemello_rientrante->rescuer->y;
                        mtx_unlock(&mutex_array_gemelli);

                        char buf_rescuer[30];
                        snprintf(buf_rescuer, sizeof(buf_rescuer), "Soccorritore %d", gemello_rientrante->id);
                        sprintf(log_msg_buffer, "Soccorritore %d di tipo '%s' per emergenza %s rientra in base", gemello_rientrante->id, gemello_rientrante->rescuer->rescuer_type_name, id_emergenza_log);
                        log_message(LOG_EVENT_RESCUER_STATUS, buf_rescuer, log_msg_buffer);

                    }
                }
            }
            if(soccorritore_tornati_base == current_nodo_emergency->data.resquer_cont){
                break;  //rientrati tutti
            }     
        }

        free(sulla_scena);  //usato per scena e rientro

        if(!possibile || shutdown_flag){
            //io per sicurezza rimetto tutti a idle
            log_message(LOG_EVENT_EMERGENCY_STATUS, id_emergenza_log, "Shutdown nel rientro dei soccorritori in base");

            mtx_lock(&mutex_array_gemelli);
            for(int i = 0; i<current_nodo_emergency->data.resquer_cont; i++){
                if(current_nodo_emergency->data.rescuer_dt[i] != NULL){
                    rescuer_digital_twin_t* gemello_finale = current_nodo_emergency->data.rescuer_dt[i];
                    gemello_finale->status = IDLE;
                    gemello_finale->x = gemello_finale->rescuer->x;
                    gemello_finale->y = gemello_finale->rescuer->y;
                }
            }
            mtx_unlock(&mutex_array_gemelli);
        }

        sprintf(log_msg_buffer, "Thread #%d, emergenza %s. Gestione terminata, soccorritori tornati in base", thread_id_log, id_emergenza_log);
        log_message(LOG_EVENT_GENERAL_INFO, id_emergenza_log, log_msg_buffer);

        //pulisco
        if(current_nodo_emergency->data.rescuer_dt != NULL){
            free(current_nodo_emergency->data.rescuer_dt);
            current_nodo_emergency->data.rescuer_dt = NULL;
        }
        free(current_nodo_emergency);
        current_nodo_emergency = NULL;





    }
    
    sprintf(log_msg_buffer, "Thread Gestore Emergenze #%d in terminazione.", thread_id_log);
    log_message(LOG_EVENT_GENERAL_INFO, "Thread gestore", log_msg_buffer);

    free(thread_args);
    return 0;
}



int main(void){

    char log_msg_buffer[LINE_LENGTH + 250];

    //inizializzo logger
    if(!init_logger(NOME_LOGGER)){
        return EXIT_FAILURE;    //non posso scrivere su file, duh
    }

    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Sistema di Emergenze avviato");


    
    //per gestione segnali
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore_segnali;

    if(sigaction(SIGINT, &sa, NULL) == -1){
        sprintf(log_msg_buffer, "Errore nella registrazione del gestore per SIGINT: '%s'", strerror(errno));
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
        pulizia_e_uscita(false,false,false,false,false, false);
        return EXIT_FAILURE;
    }


    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Gestore per SIGINT (Ctrl+C) registrato");




    //flag per tracciare lo stato di inizializzaione per pulizia (a false)
    bool config_fully_loaded = false;
    bool digital_twins_inizializzati = false;   
    bool message_queue_open = false;
    bool sync_inizializzate = false;
    bool digital_twins_mutex_inizializzata = false;
    bool id_gen_mutex_initialized = false;


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
        pulizia_e_uscita(false, false, false, false, false, false);     //nessuno dei due caricato in teoria
        return EXIT_FAILURE;
    }

    //parsing soccorritori
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizio configurazione soccorritori da rescuers.conf");
    if(!parse_rescuers("rescuers.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di rescuers.conf");
        pulizia_e_uscita(true, false, false, false, false, false);  //true perchè env.conf potrebbe essere potenzialmente ok
        return EXIT_FAILURE;
    }

    //parsing emergenze
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Inizio configurazione emergenze da emergency_types.env");
    if(!parse_emergency_types("emergency_types.conf", &global_system_config)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Errore irreversibile nel parsing di emergency_types.conf");
        pulizia_e_uscita(true, false, false, false, false, false);
        return EXIT_FAILURE;
    }

    //andato tutto bene
    config_fully_loaded = true;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Configurazione avvenuta con successo");


    //controllo configurazione basic
    
    //no gemelli digitali
    if((global_system_config.total_digital_twin_da_creare == 0) && (global_system_config.rescuer_type_num > 0)){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Presenti tipi di soccorritori ma nessun gemello digitale");
        pulizia_e_uscita(config_fully_loaded, false, false, false, false, false);
        return EXIT_FAILURE;
    }

    //coordinate
    for(int i = 0; i<global_system_config.rescuer_type_num; i++){
        rescuer_type_t *resc = &global_system_config.rescuers_type_array[i];

        if((resc->x) < 0 || (resc->y) < 0 || (resc->x >= global_system_config.config_env.grid_width) || (resc->y >= global_system_config.config_env.grid_height)){
            sprintf(log_msg_buffer, "Coordinate base del soccorritore '%s' fuori dalla griglia", resc->rescuer_type_name);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, false, false, false, false);
            return EXIT_FAILURE;
        }
    }


    //ora inizializzo i gemelli digitali
    if(global_system_config.total_digital_twin_da_creare > 0){
        global_gemelli_array = (rescuer_digital_twin_t*)malloc(global_system_config.total_digital_twin_da_creare * sizeof(rescuer_digital_twin_t));
        if(!global_gemelli_array){
            sprintf(log_msg_buffer, "Fallimento allocazione memoria per %d gemelli digitali: '%s'", global_system_config.total_digital_twin_da_creare, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, false, false, false, false, false);
            return EXIT_FAILURE;
        }
        
        digital_twins_inizializzati = true;
        sprintf(log_msg_buffer, "Allocato spazio per %d gemelli digitali", global_system_config.total_digital_twin_da_creare);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);

        int indice_gemello = 0;
        for(int i = 0; i<global_system_config.rescuer_type_num; i++){
            rescuer_type_t* tipo_corrente = &global_system_config.rescuers_type_array[i];
            int istanze_da_fare = global_system_config.instances_per_rescuer_type[i];   //array che mi ero fatto per quanti ad ogn tipo
            //sarebbe stato meglio mettelo dentro il tipo stesso ma prof non mi risponde

            for(int j = 0; j<istanze_da_fare; j++){
                rescuer_digital_twin_t* gemello = &global_gemelli_array[indice_gemello];
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
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, false, false);
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
    LOG_SCALL_THRDSC(thrd_ret, mtx_init(&mutex_emergenze_attesa, mtx_plain), LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer, config_fully_loaded, digital_twins_inizializzati, message_queue_open, false, false, false);
    //questa macro dunque ma tmx_init, guarda se non è thrd_success e nel caso stampa il log_msg_buffer, pulisce e fa EXIT_FAILURE

    //in questo punto dovrei fare la stessa cosa con cnd_init ma il problema è che non posso
    //chiamare la funzione pulizia_e_uscita per questa con sync_loaded = true
    //visto che proverebbe a distruggerere una cond var non inizializzata
    //dovrei fare un altra macro ma ritengo 9 parametri già troppi

    if(cnd_init(&cond_nuova_emergenza) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallita inizializzazione di cond var per coda di emergenze");
        mtx_destroy(&mutex_emergenze_attesa);
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, false, false, false);
        return EXIT_FAILURE;
    }
    sync_inizializzate = true;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Sistemi di sincronizzazione inizializzati");


    if(mtx_init(&mutex_array_gemelli, mtx_plain) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallita inizializzazione di mutex per gemelli digitali");
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, false, false);
        return EXIT_FAILURE;
    }
    digital_twins_mutex_inizializzata = true;
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Mutex per array gemelli digitali inizializzata");

    if (mtx_init(&mutex_generatore_id, mtx_plain) != thrd_success) {
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallita inizializzazione di mutex per generazione id");
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata, false);
        return EXIT_FAILURE;
    }
    id_gen_mutex_initialized = true; // Imposta il flag
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Mutex per generatore ID emergenze inizializzato.");



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
    if(mq_desc_globale == (mqd_t)-1){
        if(errno == EEXIST){    //coda esiste già
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Main", "Coda messaggi già esistente, apro");
            mq_desc_globale = mq_open(nome_mq_queue, O_RDONLY);
        }
        if(mq_desc_globale == (mqd_t)-1){    //ha fallito di nuovo
            sprintf(log_msg_buffer, "Fallimento apertura coda con nome %s: '%s'", nome_mq_queue, strerror(errno));
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, false, sync_inizializzate, digital_twins_mutex_inizializzata, id_gen_mutex_initialized);
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
        pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata, id_gen_mutex_initialized);
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

            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata, id_gen_mutex_initialized);
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
            for(int j = 0; j < i; j++){ // Solo i thread gestori creati con successo
                thrd_join(array_id_gestori[j], NULL);
                free(array_args_gestori[j]); // Libera gli argomenti dei thread joinati
            }
            free(array_args_gestori[i]);
            pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata, id_gen_mutex_initialized);
            return EXIT_FAILURE;
        }
        sprintf(log_msg_buffer, "Thread gestore #%d avviato", i);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
    }


    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Tutti i thread gestori avviati");
    
        while (!shutdown_flag) {
        //sleep interrompibile da segnali
        struct timespec main_sleep_ts = {.tv_sec = 1, .tv_nsec = 0};
        thrd_sleep(&main_sleep_ts, NULL);
    }


    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Invio shutdown ai tread");
    cnd_broadcast(&cond_nuova_emergenza);   //sveglio i gestori

    


    //aspetto terminazione del thread ascoltatore e controllo valore di uscita
    int res_listener;   //intero
    //anche qui potrei proteggere con macro
    if(thrd_join(id_thread_listener, &res_listener) != thrd_success){
        log_message(LOG_EVENT_GENERAL_ERROR, "Main", "Fallimento di join thread ascoltatore");
    }else{
        sprintf(log_msg_buffer, "Thread ascoltatore terminato: '%d'", res_listener);
        log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
    }

    for(int i = 0; i<NUM_THREADS_GESTORI; i++){
        if(thrd_join(array_id_gestori[i], &res_listener) != thrd_success){
            sprintf(log_msg_buffer, "Fallimento di join thread su gestore #%d", i);
            log_message(LOG_EVENT_GENERAL_ERROR, "Main", log_msg_buffer);
        }else{
            sprintf(log_msg_buffer, "Thread gestore #%d terminato", i);
            log_message(LOG_EVENT_GENERAL_INFO, "Main", log_msg_buffer);
        }
    }
    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Tutti i threadsono stati terminati");




    log_message(LOG_EVENT_GENERAL_INFO, "Main", "Pulizia risorse");

    if(message_queue_open){ 
        char nome_unlink[LINE_LENGTH + 2];
        
        //controllo sia lo stesso usato nella open
        if(global_system_config.config_env.queue_name[0] != '/'){
            snprintf(nome_unlink, sizeof(nome_unlink), "/%s", global_system_config.config_env.queue_name);
        }else{
            strncpy(nome_unlink, global_system_config.config_env.queue_name, sizeof(nome_unlink)-1);
            nome_unlink[sizeof(nome_unlink)-1] = '\0';
        }

        if(mq_unlink(nome_unlink) == -1){
            sprintf(log_msg_buffer, "Fallita unlink per coda '%s': %s", nome_unlink, strerror(errno));
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Main_Shutdown", log_msg_buffer);
        }else{
            sprintf(log_msg_buffer, "Unlink effettuata su '%s'", nome_unlink);
            log_message(LOG_EVENT_MESSAGE_QUEUE, "Main_Shutdown", log_msg_buffer);
        }
    }

    pulizia_e_uscita(config_fully_loaded, digital_twins_inizializzati, message_queue_open, sync_inizializzate, digital_twins_mutex_inizializzata, id_gen_mutex_initialized);
    //il logger viene chiuso in cleanup

    printf("Sistema terminato correttamente\n");
    return EXIT_SUCCESS;
}

