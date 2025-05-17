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

    2.1 
![alt text](image-2.png)
    



