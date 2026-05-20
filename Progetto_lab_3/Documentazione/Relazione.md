# Documentazione Progetto: Connections (Architettura e Flussi)

## 1. Struttura delle Cartelle

Il progetto è organizzato in una struttura modulare all'interno della root `Progetto/`. Di seguito il dettaglio di ogni directory e dei suoi contenuti:

*   **`src/`**: Contiene tutto il codice sorgente Java diviso per macro-componenti.
    *   **`src/client/`**: Codice esclusivo per il Client.
        *   `ClientMain.java`: Entry-point dell'applicativo client e gestore della Console (CLI).
        *   `network/`: Contiene la classe `NioClient` che gestisce l'infrastruttura di rete TCP/UDP non bloccante.
    *   **`src/server/`**: Codice esclusivo per il Server.
        *   `ServerMain.java`: Entry-point del server.
        *   `game/`: Logica del gioco. `WordDataset` analizza i file JSON, `Game` rappresenta la singola partita in corso e `GameManager` gestisce il ciclo di vita (timer e rotazione).
        *   `model/`: Strutture dati, tra cui la classe `User` che traccia login, punteggi, errori e proposte.
        *   `network/`: Gestione del protocollo di rete del server. `NioServer` accetta i socket e smista il lavoro, mentre `CommandProcessor` elabora i comandi JSON in ingresso e produce una JSON in uscita.
        *   `util/`: Classi di supporto. Contiene `StorageManager` per le operazioni I/O sul file degli utenti.
    *   **`src/common/`**: Classi condivise sia dal Client che dal Server, per evitare duplicazioni.
        *   `config/`: Contiene `ConfigReader` per analizzare agilmente file `.properties`.
        *   `network/`: I DTO (Data Transfer Object) ovvero `JsonRequest` e `JsonResponse` che vengono serializzati e mandati via TCP.
        *   `util/`: File `Constants.java` con variabili costanti come nomi delle operazioni e codici errore.
*   **`lib/`**: Librerie esterne necessarie all'esecuzione. Contiene `gson-2.10.1.jar` usato per l'elaborazione del JSON.
*   **`config/`**: File di proprietà e configurazione.
    *   `client.properties`: Parametri IP e Porta di destinazione.
    *   `server.properties`: Parametri IP locale, porta d'ascolto, durata singola partita (timer) e intervallo del salvataggio automatico.
*   **`data/`**: Contiene i dati persistenti caricati dal sistema a runtime.
    *   `words.json`: L'enorme dataset originale contenente migliaia di configurazioni di partita (le 16 parole suddivise in gruppi).
    *   `users.json`: Generato al volo dal Server. Salva i progressi degli utenti, le password e lo storico in un formato facile da ripristinare.
*   **`out/`**: (*generata allo script*) Cartella temporanea che conserva i file compilati `.class` prima di inserirli nei `.jar`.
*   **`Documentazione/`**: Contiene la specifica del progetto e documentazione generata (tra cui questo file).

---

## 2. Flusso di Esecuzione: `ServerMain`

Il server opera come un fulcro centrale che smista e coordina. Il suo avvio segue questo processo lineare:

1.  **Avvio e Configurazioni**: `ServerMain` inizia creando un'istanza di `ConfigReader` e legge `server.properties` per capire su quale porta mettersi in ascolto e i vari path per il salvataggio e il dataset.
2.  **Caricamento Parole**: Invoca `WordDataset.loadDataset(path)`. Questa classe, sfruttando Gson, va a convertire l'intero array JSON `words.json` in una lista mappata in memoria di oggetti `GameData` e i loro gruppi.
3.  **Inizializzazione NIO Server**: Viene istanziato il `NioServer(port)`. Questo allocherà le strutture di base: un `Selector` NIO, le *ConcurrentHashMap* per memorizzare sia le associazioni "Socket-Utente" (chi è loggato in quale socket) sia gli utenti in senso assoluto (registrati). Viene inoltre creato in NioServer un **CachedThreadPool** (i Worker).
4.  **Avvio Ciclo di Gioco**: Viene istanziato il `GameManager`. Al richiamo di `gameManager.start()`, la classe estrae la partita all'indice 0, fa uno "shuffle" casuale delle sue 16 parole e crea un `ScheduledExecutorService`. Questo servizio ha il compito di svegliarsi allo scadere dei minuti decisi per partita:
    *   *Allo scadere*, invocherà tramite il NioServer un `broadcastGameEnd()` che manderà in UDP un JSON "GAME_ENDED" a tutti gli IP loggati. Successivamente la partita avanzerà all'indice successivo ricaricando il timer in modo ciclico per infinito.
5.  **Avvio della Persistenza**: Si avvia `PersistenceManager`. Per prima cosa legge l'eventuale `data/users.json` ricreando le vecchie utenze (se è un riavvio dopo un crash). Successivamente usa uno ScheduledThread per "dormire" e risvegliarsi ogni *X* minuti per salvare uno snapshot in JSON della mappa utenti in memoria, in totale background.
6.  **Loop di Rete (Acceptor)**: Infine, il Main thread chiama `server.start()` che è bloccante. Qui il `Selector` si blocca nel loop `select()`. 
    *   Quando riceve una nuova connessione la accetta.
    *   Quando arrivano dati testuali via TCP (già codificati in JSON), invia un *Runnable* al **CachedThreadPool**.
    *   Un Thread del pool prenderà il testo JSON, lo darà a `CommandProcessor` il quale estrarrà il campo "operation" (es. `login`), applicherà la logica prelevando il lock di Sync per sicurezza se modifica l'utente e genererà il `JsonResponse` sputandolo indietro nel SocketChannel asincronamente.

---

## 3. Flusso di Esecuzione: `ClientMain`

L'architettura del Client unisce interattività utente classica con una rete non-bloccante, separando il lavoro in due thread principali.

1.  **Avvio e Setup**: `ClientMain` parte caricando `client.properties` (IP Server e Porta). Crea poi un'istanza di `NioClient`.
2.  **Preparazione Canali NIO**: Viene richiamato `client.connect()`. All'interno del `NioClient` accadono due cose fondamentali:
    *   Si apre un `SocketChannel` (connessione TCP) verso l'indirizzo del Server.
    *   Si apre un `DatagramChannel` (UDP) in ascolto effettuando un *bind(0)* locale, permettendo al sistema operativo di agganciargli una porta *effimera* e locale casuale.
3.  **Lancio Background Thread di Rete**: Si lancia un Thread aggiuntivo che esegue esclusivamente `client.run()`. Questo Thread starà intrappolato in un loop con `selector.select()`.
    *   *Se legge dal canale TCP*: riceve le risposte standard ai comandi inviati al server, li formatta a schermo e ristampa il prompt `> ` della console per fluidità.
    *   *Se legge dal canale UDP*: significa che è arrivata la notifica asincrona globale (fine tempo della partita) ignorando ciò che si sta scrivendo sulla console. Lo stampa a schermo.
4.  **Scanner Console Loop (Main Thread)**: Il thread principale del programma, arrivato qui, entra nel loop `while(true)`. 
    *   Si mette in attesa dello `Scanner(System.in)`. Questo è *bloccante*, il che è desiderato poiché l'utente sta scrivendo.
    *   Cattura l'input (es. `propose p1 p2 p3 p4`). Effettua una validazione lessicale in loco (verifica quanti token sono, che i comandi abbiano senso o argomenti giusti).
    *   **Invia il Payload**: Popola l'oggetto Java `JsonRequest` e chiama `client.sendRequest(req)`. Sotto il cofano, Gson converte in JSON, lo spinge nel `SocketChannel` e l'esecuzione torna allo Scanner in attesa del comando successivo, mentre il Background Thread (punto 3) resterà passivo in attesa della risposta che il Server produrrà.

> [!NOTE]
> Il **Trucco UDP**: Nel caso dell'operazione "login", il Main Client rileva automaticamente la propria porta UDP tramite il metodo `client.getUdpPort()` e lo integra nel body JSON della request di login. Il server accetta il login e capisce che l'IP remoto è associato a *quella* particolare porta UDP per mandare avvisi in broadcast, senza l'uso di procedure complesse (heartbeats, protocolli di scambio ausiliari o simili).
