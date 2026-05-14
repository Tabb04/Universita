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

6. CommandProcessor.java
    A. Primo if, sembra che i comandi non definiti sono controllati in server, ma anche in client mi sa


APPUNTI PER RELAZIONE
- AI per if (this.currentFoundGroups == null), inizializzazione lazy di GSON dove bypassa il costruttore
- Scelta del threadpool, forse fixed con calcolo delle cpu sarebbe stato meglio ma boh
- Ricorda di rimuovere i commenti /**
- La porta UDP la faccio mandare a login
- Se vuoi sostituisci "common" come nome della cartella
- Guardare come fare in modo che Vscode faccia gli import in automatico


AL MOMENTO SONO IN SERVERMAIN -> NIOSERVER -> COMMANDPROCESSOR