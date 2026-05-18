1. ConfigReader.java
    A. Il tipo Properties viene popolato come una hashtable da un file.
    B. Integer.parseInt(value); converte una stringa in valore

2. ServerMain
    A. Se vuoi cambiare da dove prendo le parole cambia datasetPath

3. WordDataset.java
    A. La classe TypeToken è un trucco inventato da Google: quando compili il codice, il programma "dimentica" i tipi generici contenuti nelle liste. A runtime List<GameData> viene vista come una List<> generica.

    B. Per implementazione il programma carica TUTTE le parole in Ram subito, per velocità.
    Si può dire che è inefficiente o efficiente in base a come si guarda.

    C. Commentato

4. User.java
    A. Tutto appuntatom, fatto. Ricorda inizializzazione lazy di Gson che skippa costruttore.

5. NioServer.java
    A. Molto ispirato all'assigment 9
    
    B. Perché if(bytesRead == -1) vuol dire che il client ha chiuso la connessione. Nelle connessioni
       quando smetto di mandare si indica con 0, -1 è il FIN

    C. workerPool.submit(() -> processMessage(clientChannel, message));
       Uso una lambda expression. Di solito mandavo un oggetto che era un istanza di un runnable, qui è 
       praticamente la stessa cosa. Mando un oggetto che non prende parametri e ha come metodo run
       ovverridato fare la funzione processMessage

    D. JsonRequest request = gson.fromJson(message, JsonRequest.class); riga ~ 204
       Trasforma una stringa java in un oggetto per reflection

    E. (Non ho ancora fatto quella parte)
        Quando faccio il broadcast della nuova partita il messaggio di fine arrivava dopo quello di inizio nuova.
        Per ovviare a questo problema ho dovuto inserire una sleep in quello della nuova partita. Il reset della 
        partita comunque avviene prima di tutto (non serve a tanto ma potrebbe manadre una proposta in quel secondo?)
        Potenziali soluzioni senza sleep: 1. Usare protocollo TCP per entrambi, così ho garanzia di ordine di consegna
                                          2. Bufferizzare nel client per riordinare
                                          3. Chiamare in GameManager.java la broadcast del fine partita e poi fare startNextGame(). Questo però non è una soluzione sicura perché essendo due protocolli diversi lo scheduler di rete non assicura l'ordine dei due messaggi

6. CommandProcessor.java
    A. Primo if, sembra che i comandi non definiti sono controllati in server, ma anche in client mi sa

    B. Utilizzo di fine-grained-locking (usare synchronized)

    C. Per ogni operazione utilizzo il channel e guardo a che utente è associato

    D. Oggetto "Data" usato sempre per il payload (non sempre usato)

    E. RequestPlayerStats si può fare solo per se stessi mentre leaderboard posso richiedere anche specificatamente un altro
       utente con un po' meno dettagli


7. JsonRequest.java
    A. Ho fatto campi specifici che vengono utilizzati solo per alcuni tipi di richieste. Es. Non riutilizzo gli stessi campi del login anche per fare l'aggiornamento delle password

    B. UpdateCredentials se non voglio aggiornare un campo basta mettere un dash


APPUNTI PER RELAZIONE
- AI per if (this.currentFoundGroups == null), inizializzazione lazy di GSON dove bypassa il costruttore
- Scelta del threadpool, forse fixed con calcolo delle cpu sarebbe stato meglio ma boh
- Ricorda di rimuovere i commenti /**
- La porta UDP la faccio mandare a login
- Se vuoi sostituisci "common" come nome della cartella
- Guardare come fare in modo che Vscode faccia gli import in automatico
- Ho locckato tante operazioni dietro a login come richiesto da specifica
- Riguardare bene come stampare un oggetto json
- Cosa cambierebbe nell'implementazione se gli utenti fossero caricati a richiesta invece che subito tutti in memoria? Dovrei cambiare tutte le funzioni?
- Correggere import soprattutto per le altre cartelle

AL MOMENTO SONO IN SERVERMAIN -> NIOSERVER


Modifiche per correzione di autologin a prossima partita

- [x] Aggiungere `broadcastNewGameStart` in `NioServer.java`
- [x] Chiamare `broadcastNewGameStart` dal timer di `GameManager.java`
- [x] Implementare auto-iscrizione "lazy" in `CommandProcessor.java`
- [x] Ricompilare e verificare il progetto
