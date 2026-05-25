# Piano di Test per Connections

Questo documento fornisce scenari di test manuali per verificare che tutte le operazioni, *rinominate esattamante come nel file `Istruzioni.md`*, funzionino in maniera coerente.

## Preparazione
Avvia il server in un terminale separato:
```bash
java -jar Server.jar
```

---

## Test Case 1: Gestione Account (updateCredentials completo)
**Obiettivo:** Testare `register`, `login`, e l'aggiornamento simultaneo o parziale di username e password.

**Terminale Client:**
1. Esegui: `java -jar Client.jar`
2. **Registrazione:** `register mario pass`
   - *Atteso:* Successo.
3. **Login:** `login mario pass`
   - *Atteso:* Messaggio di benvenuto, le 16 parole della partita.
4. **Cambio solo password:** `updateCredentials mario pass - nuovaPass`
   - *Atteso:* Password aggiornata con successo (il `-` indica di lasciare invariato il nome).
5. **Tentativo login con vecchia password:** Apri un altro client e fai `login mario pass`
   - *Atteso:* Errore 102 credenziali errate.
6. **Cambio Username e Password:** Nel primo client, esegui `updateCredentials mario nuovaPass luigi passFinale`
   - *Atteso:* Credenziali aggiornate. Il tuo username ora è `luigi`.

---

## Test Case 2: Gameplay Base (submitProposal)
**Obiettivo:** Verificare che il comando principale di gioco risponda e calcoli gli errori come previsto.

**Terminale Client:** (loggato come *luigi*)
1. Esegui: `submitProposal GATTO CANE TOPO PESCE` (usa parole mostrate al momento del login, in uppercase)
   - *Atteso:* `[SERVER] -> {"result":"wrong" ...}` se errate (con penalità -4), oppure `{"result":"correct" ...}` con incremento punteggio di +6.
2. Esegui una proposta malformata (es. una parola non presente in lista o duplicata): `submitProposal GATTO GATTO A B`
   - *Atteso:* Il server risponde con un errore ("Proposta malformata...") ma i tuoi tentativi/errori e punti rimangono invariati.
2. Invia 5 o più parole per testare l'errore lato client: `submitProposal A B C D E`
   - *Atteso:* `Errore. Uso: submitProposal <w1> <w2> <w3> <w4>` dalla console senza neanche interpellare il server.

---

## Test Case 3: Recupero Informazioni e Statistiche (Le 4 request*)
**Obiettivo:** Testare i nuovi comandi previsti nella sezione 2.1 e Appendice.

**Terminale Client:**
1. **Richiesta info partita attuale:** `requestGameInfo`
   - *Atteso:* Ritorna il tempo rimanente, le parole mischiate, le proposte corrette indovinate finora, i tuoi errori e il punteggio corrente.
2. **Richiesta statistiche partita attuale:** `requestGameStats`
   - *Atteso:* Ritorna il numero di giocatori attualmente in gioco, quanti hanno finito, quanti hanno vinto e il tempo rimanente.
3. **Richiesta player stats storiche:** `requestPlayerStats`
   - *Atteso:* Ritorna un JSON completo in stile NYT con `puzzlesCompleted`, `winRate`, `lossRate`, `currentStreak`, `maxStreak`, `perfectPuzzles` e `mistakeHistogram`. Si aggiornano allo scadere del timer di ogni partita a cui si partecipa.
4. **Classifica Top Players:** `requestLeaderboard topPlayers 5`
   - *Atteso:* JSON con un array ordinato per punteggio globale di tutti i tempi.
5. **Richiesta info per partita passata (storico):** `requestGameInfo <GameID_Passato>`
   - *Atteso:* Se la partita è terminata, ritorna la corretta assegnazione delle parole ai 4 gruppi, oltre a indicare il punteggio, gli errori e l'esito della tua prestazione in quella specifica partita passata.
6. **Richiesta statistiche globali per partita passata:** `requestGameStats <GameID_Passato>`
   - *Atteso:* Ritorna il totale dei partecipanti di quella partita, quanti hanno concluso, quanti hanno vinto e il punteggio medio generale.

---

## Test Case 4: Flush Statistiche e Broadcast UDP
**Obiettivo:** Verificare che il passaggio tra una partita e l'altra storicizzi i dati globali.

**Setup:**
- Nel terminale server, se imposti il timer basso (es. `server.timer.seconds=30` in `server.properties`) vedrai più in fretta il comportamento.

**Terminale Client:**
1. Assicurati di aver inviato almeno una proposta che abbia causato una modifica di punteggio (in positivo o negativo, es. -4 punti per errore).
2. **Attendi il messaggio UDP**: Il terminale riceve `[BROADCAST UDP] -> {"event":"GAME_ENDED","gameId": ...}` senza fare nulla (viene stampato a console).
3. **Verifica il travaso del punteggio e delle NYT Stats:** Subito dopo, fai `requestPlayerStats`.
   - *Atteso:* I dati (es. `puzzlesCompleted`, `winRate`, `mistakeHistogram`) sono stati calcolati in base alla partita appena conclusa.
4. **Verifica lo storico della partita conclusa:** Usando il `gameId` mostrato nel broadcast, esegui `requestGameStats <gameId>` e `requestGameInfo <gameId>`.
   - *Atteso:* Entrambi i comandi mostreranno i dati storici della partita terminata.

Questi comandi sono in linea al 100% con le richieste di `Istruzioni.md` e testano sia il payload JSON che le funzionalità architetturali.
