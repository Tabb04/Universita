Documentazione per progetto esitivo di Laboratorio2

Studente: Mirco Mauro Tabarracci
Matricola: 674556

Il progetto implementa un programma multithread che simula un gestore di emergenze
simulate su una griglia bidimensionale.
Il "server" riceve emergenze da un client tramite una coda messaggi POSIX, assegna risorse in modo
concorrente tenendo conto del numero delle risorse disponibili.
Ogni azione/errore rilevante viene scritta su un file di logging chiamato emergency_system_log


1. Parsing
    Come specificato sul pdf l'ambiente, emergenze e soccorritori vengono registrati
    tramite 3 parser.
    
    1.1 parse_env.c
        Questo parser prova a inizializzare l'ambiente simulato provando a fare tutti
        i controlli possibili.
        Oltre a fare i soliti controlli su validità dei parametri, controlla che il nome della coda non superi la dimensione del buffer, controlla che il nome passato non contenga caratteri '/' visto che l'unico all'inizio sarà aggiunto dal codice e controlla che i valori per la griglia siano sensati

    1.2 parse_rescuers.c
        Questo parser invece salva i tipi di emergenze definiti nel file rescuers.conf
        provando anche lui a fare tutti i controlli possibile.
        Per prima cosa prova a contare i potenziali soccorritori validi guardando se 
        combaciano con la formattazione data dal pdf, tramite l'utilizzo della
        funzione sscanf con sscanf(...,  [%[^]]] [%d] [%d] [%d;%d], ...) dove se vengono
        trovati tutti e 5 parametri incremento il counter dei possibili.
        Questo viene fatto in modo tale che si possa prima allocare un array sufficientemente
        grande per i soccorritori
        Allora si può fare il parsing nuovamente sta volta facendo i controlli sui valori
        assegnabili come numeri non negativi, nomi non troppo lunghi ecc.

    1.3 parse_emergency_types
        Questo parser molto brevemente si comporta nello stesso modo di parse_rescuers.c
        usando sscanf per una stima iniziale con sscanf(" [%[^]]] [%hd]") per poi fare il parsing basandosi su ";" per tokenizzare ogni soccorritore. Fa anche lui tutti
        i controlli sulle validità dei valori.


2. Strutture dati
    Le strutture dati sono situate nel file data_types.h e sono quelle date nel pdf.
    Ho poi aggiungo delle strutture dati di supporto:

    2.1 environment_t
        Struttura dati abbastanza basilare per salvare i dati dell'ambiente che abbiamo
        parsato

    2.2 system_config_t
        Struttura dati per aggregare tutta la configurazione letta dai 3 file e contiene:
        Un campo di tipo environment_t (vedi sopra)
        Un array di rescuer_type_t che contiene i tipi di soccorritore parsati (con le loro
        caratteristiche), un campo int che dice quanti tipi di soccorritori abbiamo e un campo
        int* che dice quante unità per ogni tipo di soccorritore abbiamo
        Poi abbiamo lo stesso con un array di emergenze e il count di quanti tipi ne abbiamo.
        E infine abbiamo un counter su quante unità (gemelli digitali) abbiamo in totale.

    2.3 emergency_node_t
        Questa struttura dati è definita e utilizzata nel main ed ha lo scopo di implementare
        una lista linkata per le emergenze in attesa da essere gestite dai gestori.
        Ha i campi data di tipo emergency_t che contiene i dati dell'emergenza, un id di 
        tipo di char per un identificativo univoco e un puntatore al nodo prossimo nella lista.
        Per poi essere gestito correttamente viene fatto un puntatore alla testa e coda della 
        lista e un mutex per accesso esclusivo.

    2.4 Altre
        Sono presenti anche altre piccole strutture dait di supporto come rescuer_candidate_t
        che servono nella funzione dei gestori emergenze per aiutare a selezionare i soccorritori migliori e thread_args_t per il passaggio di argomenti ai threads.


3. Logging
    Tutto il logging su file e eventuali accorgimenti sono gestiti dalle funzioni definite nel
    file logger.c
    Il logging su file contiene il timestamp del logging, l'id del chiamante del logging, il tipo di evento definito tra i campi dell'enum nell'header (enum che segue la richiesta del pdf) trasformato
    in stringa dalla funzione log_event_type_to_string e il messaggio passato.


4. Main
    Il main svogle il lavoro di server interattivo per la gestione delle emergenze

    4.1 Inizializzazioni
        Comincia il Main con inizializzare tutti i flag per la pulizia per poi avviare le funzioni di parsing.
        Dopo ciò procede con l'inizializzazione dei gemelli digitali allocando spazio tramite il counter
        globale delle unità.
        Poi inizializza tutti i sistemi di sicronizzazione come mutex e condition variables.
        Infine dopo aver aperto e inizializzato la coda messaggi procede con la creazione del thread
        ascoltatore della coda messaggi e dei thread gestori delle emergenze

    4.3 Thread ascoltatore
        Il thread ascoltatore utilizza la funzione message_queue_ascoltatore_func.
        Viene utilizzata invece di una mq_timedrecive invece di una mq_recive in modo tale che
        si possa svolgere una corretta terminazione del programma. (spiegato bene nella gestione dei segnali)
        La funzione quindi fa i controlli sul messaggio ricevuto e se tutto è corretto alloca
        spazio per emergency_node_t, assegna i suoi valori e con la mutex ottenuta aggiunge il nodo
        alla coda delle emergenze in attesa

    4.4 Thread Gestore
        Il thread Gestore utilizza la funzione gestore_emergenze_func.
        Comincia col controllare se sono disponibili delle emergenze nella lista delle emergenze in
        attesa, se si la preleva e rilascia il lock. Allora la funzione calcola il deadline, e 
        crea un array dove saranno memorizzati i migliori candidati trovati. Poi itera sul numero
        di soccorritori richiesti dall'emergenza e per ogni tipo di emergenza aquisisce il lock
        dell'array dei gemelli digitali, se un gemello ha il tipo giusto calcola il tempo di arrivo
        e lo aggiunge all'array dei candidati (dove ho il candidato e il tempo di arrivo).
        A questo punto ho l'array con tutti i candidati possibili che può essere ordinato in base
        al tempo potenziale d'arrivo e posso selezionare i primi tanti quanti ne sono richiesti.
        A questo punto confronto il tempo di arrivo del più lento con il deadline prestabilito e se
        non è sufficiente assegno TIMEOUT e esco.
        A questo punto se tutto è andato bene posso assegnare i gemelli digitali all'emergenza (e di
        conseguenza aggiornare lo stato dei gemelli nell'array dei gemelli globale). Ho provato anche
        a controllare se c'è stato un conflitto, ergo se un soccorritore è stato preso da qualcun'altro nel frattempo (potrei anche fare tutto fino a qui tenendo la lock ma sembra poco efficiente).
        Se sono arrivato qui posso iniziare le simulazioni e lo faccio facendo una thrd_sleep che durerà
        quanto il soccorritore più lento ci mette ad arrivare (faccio sleep a scaglioni di un secondo per controllare che non ci sia stata un interruzione). Quando sono arrivati tutti posso simulare il 
        lavoro sul posto nello stesso modo e fare ancora lo stesso per il rientro in base. Quando sono
        rientrati tutti posso aggiornare lo stato dei gemelli e uscire.

    4.5 Pulizia
        Tutta la pulizia prima di uscite per errore o di terminazione sono gestite dalla funzione
        pulizia_e_uscita() definita nel main.
        La funzione esegue le necessarie pulizie basandosi su parametri booleani passati.
        In ordine il primo parametro guarda se la variabile di tipo system_config_t che contiene
        tutti i parametri di configurazione dell'ambiente è stata inizializzata.
        La seconda

     





