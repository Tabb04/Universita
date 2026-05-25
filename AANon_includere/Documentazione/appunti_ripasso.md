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
- Se vuoi cambiare i percorsi devi cambiare anche all'inzio del main server dove vengono caricati i file di proprietà
- Messaggi del server hanno tutti la formattazione del tipo [Thread che esegue]: ...
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
- Ricorda che .addProperty(..) si usa per aggiungere valori primitivi mentre .add(...) si usa per aggiungere strutture Json complesse. Ad esempio quando stampo il messaggio di fine uso .addProperty per cose come "event: GAME_ENDED" mentre .add per cose come "groups: [{"theme": "WANE", "words:["PETER", "TAPER" ...]}] per indicare la soluzione
- Nelle parole non usa gli apicetti "'" ma usa quello delle citazioni

- Per come è nested devo avere per forza la sleep. Questo perché chiamo la nuova partita. Essa aggiorna i dati per avere lo storico pronto per il messaggio di fine, si mette a dormire un secondo e ritorna. Dopo che è ritornata parte il messaggio di fine, si sveglia l'addormatato e manda il messaggio di nuova partita. (guarda da startNextGame se vuoi capire le chiamate)
- Riguarda come sarebbe stato con caricamento di tutto in memoria
- Non è comunque molto efficiente caricare tutte le partite vecchie in memoria
- Controllare commenti che non ci siano riferimenti alle istruzioni e non ci siano commenti /*
- Scelto di implementare il clintNio con un selector




AL MOMENTO SONO IN SERVERMAIN -> GAMEMANAGER


Modifiche per correzione di autologin a prossima partita

- [x] Aggiungere `broadcastNewGameStart` in `NioServer.java`
- [x] Chiamare `broadcastNewGameStart` dal timer di `GameManager.java`
- [x] Implementare auto-iscrizione "lazy" in `CommandProcessor.java`
- [x] Ricompilare e verificare il progetto










Ecco uno schema temporale dettagliato che mostra esattamente come si comporta il sistema a partire dall'avvio del server, evidenziando le interazioni tra il **Thread Principale (Main)**, il **Thread dello Scheduler** e i **Thread dei Client (via TCP/UDP)**.

Assumiamo per l'esempio che una partita duri **40 secondi** (`gameDurationSeconds = 40`).

---

### Schema Temporale Visivo (Timeline)

```text
TEMPO (s)   THREAD PRINCIPALE (ServerMain)        THREAD SCHEDULER (GameManager)       CLIENTS (TCP/UDP)
========================================================================================================================
 t = 0s     [Avvio Server]


[ignoring loop detection]

Ecco lo schema temporale dettagliato che illustra il ciclo di vita del server a partire dall'avvio del primo gioco, mostrando l'interazione tra il **Thread di Avvio**, il **Thread dello Scheduler** (in background) e i **Client**.

Per l'esempio, ipotizziamo una partita con durata impostata a **40 secondi**.

---

### Schema della Linea Temporale

```text
TEMPO (t)     THREAD DI AVVIO (Main)          THREAD DELLO SCHEDULER (GameManager)    THREAD WORKER TCP/UDP
===========================================================================================================================
t = 0.0s      - Chiama GameManager.start()
              - startNextGame() (crea Partita 1)
              - Pianifica timer fine Partita 1 
                (in background tra 40s)
              - Il Main finisce e si mette
                in ascolto su porta TCP
---------------------------------------------------------------------------------------------------------------------------
t = 0.1s      [In attesa...]                  [Dorme e attende che scada il timer]    - I client giocano (inviano proposte)
...                                                                                   - Il server risponde via TCP
t = 39.9s                                                                             - Tutto gira sui thread worker TCP
---------------------------------------------------------------------------------------------------------------------------
t = 40.0s     [In ascolto TCP...]             - IL TIMER SCADE! Il thread si sveglia
                                              - Chiama startNextGame() (Partita 2):
                                                1. Cicla su tutti gli utenti e
                                                   aggiorna i loro punteggi storici
                                                   con resetGameState(-1)
                                                2. Carica le parole di Partita 2
                                                3. Assegna Partita 2 a currentGame
                                                4. Avvia thread TCP per notificare
                                                   "NEW_GAME_STARTED" (con sleep 1s)
                                                5. Pianifica timer fine Partita 2
                                                   (in background tra altri 40s)
                                              - Invia UDP "GAME_ENDED" (soluzione 
                                                e classifica della Partita 1)
                                              - Torna a dormire in attesa di t = 80s
---------------------------------------------------------------------------------------------------------------------------
t = 40.1s     [In ascolto TCP...]             [Dorme]                                 - I client ricevono il pacchetto UDP
                                                                                        di fine partita 1 e lo mostrano
---------------------------------------------------------------------------------------------------------------------------
t = 41.0s     [In ascolto TCP...]             [Dorme]                                 - Il thread TCP di notifica si sveglia
                                                                                        (dopo la sleep di 1s)
                                                                                      - Spedisce TCP "NEW_GAME_STARTED"
                                                                                        a tutti i client (Partita 2)
                                                                                      - I client caricano il nuovo gioco
---------------------------------------------------------------------------------------------------------------------------
t = 80.0s     [In ascolto TCP...]             - IL TIMER SCADE! Il thread si sveglia
                                              - (Il ciclo si ripete per Partita 3)
```

---

### Punti Chiave da Notare

1. **Nessun Blocco del Main**: All'avvio (`t = 0s`), una volta pianificato il primo evento, il thread di boot termina la configurazione ed entra nel loop di selezione TCP (`selector.select()`). Lo scheduler lavora in modo completamente asincrono su un altro thread.
2. **La nascita pianifica la fine**: Ogni volta che viene avviata una partita dentro `startNextGame()`, la primissima cosa che viene fatta a fine metodo è registrare l'evento di chiusura programmato a `t + 40s`. Questo rende il ciclo infinito autosufficiente.
3. **Nessuna sovrapposizione**: Visto che lo scheduler è a *singolo thread* (`SingleThreadScheduledExecutor`), se per qualsiasi motivo un'operazione di rotazione partita dovesse subire un rallentamento (es. scrittura su disco lenta), il timer della partita successiva non partirebbe "in anticipo" rischiando collisioni, ma verrebbe calcolato in modo sequenziale perfetto dal momento dell'effettiva inizializzazione della nuova partita.











RIPASSO DELLA TIMELINE DAL MAIN:
1. Avvio il main (MAIN).
2. Il main chiama ConfigReader passando il path del file delle server properties (MAIN).
3. ConfigReader usa un oggetto Properties per caricare tutte le proprietà con chiave-valore. (CONFIGREADER)
4. Il main fa .getIntProperty e .getProperty per prendere i valori di config (MAIN)
5. Il main inizializza il NioServer per ascoltare le connessioni (MAIN)
6. NioServer inizializza con la porta passata dal main e crea una CachedThreadPool e un Gson per leggere le stringhe json (NIOSERVER)

7. Il Main inizializza un GameManager passando il percorso delle parole, il tempo di una parita e il serverNio (MAIN)
8. GameManager stanzia i parametri e inizializza un ScheduledThread per gestire la fine della partita (GAMEMANGER)
9. Il GameManager conta le partite su file, e inizia una nuova partita scegliendone una causale. Poi imposta un thread schedulato. IL thread addormentato si sveglia e: Manda messaggio di fine sul terminale del server. Chiama startNextGame() la quale salva i valori della partita nello storico e usa User.resetGameState(-1) per salvare i dati dell'utente. Poi startNextGame() chiama server.broadcastNewGameStart(newGame) il quale schedula un thread tra un secondo per scrivere (nuova partita iniziata) e poi ritorna. StartNextGame nel frattempo schedula se stessa un altro thread con il timer della partita che rifarà la stessa cosa. Il thread risvegliato infine usa broadcastGameEnd per annunciare la fine della partita al client e stampare il resoconto della partita (con notifica asincrona UDP) e nel frattempo il thread schedulato tra un secondo finisce e manda il messaggio di nuovo inzio partita. Il thread risvegliato termina e ricomincia il ciclo quando si sveglia l'altro thread schedulato con il timer della partita. (GAMEMANAGER)

10. Il main dunque prende i valori della cartella con le parole e il timer di intervallo dei salvataggi automatici. (MAIN)

11. Il main passa quei valori allo StorageManager + la lista degli utenti (MAIN)

12. Lo StorageManager inizializza un thread schedulato (STORAGEMANAGER)

13. Il main dice allo StorageManager di caricare tutti gli utenti da fine nella mappa condivisa registeredUsers. (MAIN-STORAGEMANAGER)

14. Faccio lo stesso con le partite precedenti (MAIN-STORAGEMANAGER)

15. Dunque imposto il thread schedulato di StorageManager in quale ogni intervallo chiama le due funzioni saveUsers e savePastGames le quali scrivono su file gli utenti e le partite precedenti dalla memoria (MAIN-STORAGEMANAGER)

16. Imposto la cattura del SIGINT il quale chiama la funzione di salvataggio di StorageManager ed esce (MAIN)

17. Avvio il GameManager che fa quello che è scritto sopra (MAIN)

18. Avvio il NioServer (MAIN)

19. Il NioServer crea un selector, poi si mette in ascolto sulla porta passata. Poi fa il classico ciclo con i selector dove aspetta le richieste di connessione.
Quando riceve una richiesta di connessione chiama acceptConnection la quale tira fuori il canale di ascolto, fa una .accept() su di esso e ricava un oggetto SocketChannel chiamato client. Dopo di che si mette in modalità non bloccante e registra il nuovo channel per eventi di lettura (quando scriverà).

20. Per la lettura tira fuori il canale del client. Poi tira fuori il buffer collegato alla chiave e usa clientChannel.read() per scriverci dentro. Se legge -1 vuol dire che ha mandato la chiusura e verrà gestito dall'altra funzione. A questo punto fa la flip sul buffer e memorizza il messaggio in un array di byte (per la scrittura). Genero una stringa dal quel vettore di byte e uso il ThreaPool per mandare la stringa ad elaborare con la funzione processMessage. (NioServer)

21. ProcessManage usa il messaggio ricevuto e con la funzione .fromJson lo converte in un oggetto JsonRequest. A questo punto manda questo oggetto e il canale del client alla funzione CommandProcessor.process(...), la quale restituirà un oggetto di tipo JsonResponse. Questo oggetto infine sarà inivato di nuovo sul canale del client con sendResponse(clientChannel, ...) insieme al riferimento al server stesso. (NioServer)

22. In CommandProcessor arriva l'oggetto richiesta JsonRequest, il canale SocketChannel e il server. CommandProcessor smista il messaggio basandosi sul campo "operation" della richiesta e esegue una funzione di handle rispettivamente. Per operazioni che richiedono modifiche ai valori di utenti (come registrare un utente, aggiornare le credenziali) sono fatte con blocchi di sincronizzazione. Le JsonResponse possono essere success o error distinte dal campo "status". È poi presente un campo (opzionale) di codice di errore e di messaggio di errore. Infine è presente un campo di payload opzionale "data" che contiene dati (solo se success) in risposta ad una richiesta del client