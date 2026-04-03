Da scrivere
1. Segnalare la differenza tra inject e sendpacket

2. Monothread

3. Fork di github

4. Lasciato codice commentato per orientarsi

5. Non efficiente, altra opzione non bloccante forse?7

6. Lasciato anche modalità 

7. IMPORTANTE. SICCOME SPECIFICA "NOME A DOMINIO" non posso usare i filtri ma devo analizzare il payload
perché viaggia a livello applicativo. È molto inefficiente, deep packet inspection

Timeout di 1 secondo:
In sintesi, è come dire al programma: "Mettiti in ascolto, ma se per un secondo intero non vola una mosca, fai un respiro, controlla che io non ti abbia chiesto di spegnerti, e poi rimettiti in ascolto".


int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
Il primo valore è fino a che numero dei descrittori devo controllare (un po' inefficiente)
Se ad esempio fd_in è 3 e fd_out è 4, max_fd sarà 4. Passando 4 + 1 = 5, diciamo alla select di controllare gli FD 0, 1, 2, 3 e 4.
Altri due NULL dice di Ignorare i controlli sulla scrittura e di non monitorare robe che non ci interessano.

restituisce valore > 0: Almeno un file descriptor ha dei dati pronti (successo).



res = pcap_next_ex(pd_in, &header, &pkt_data);
Primo handler, secondo dove salvo header, terzo puntatore ai dati.






È una domanda fondamentale! Capire come funziona la `select()` segna il passaggio da "scrivo codice base" a "so come far interagire in modo efficiente il mio programma con il sistema operativo". 

Per capirne la logica, usiamo una metafora. 

Immagina di essere il portiere di un albergo con **due telefoni** sulla scrivania (le nostre due interfacce di rete). 
Se tu usassi `pcap_loop` su un telefono, staresti letteralmente con la cornetta incollata all'orecchio aspettando che qualcuno parli. Se nel frattempo squillasse il secondo telefono, tu lo ignoreresti completamente. 

La funzione `select()` è come un **centralino intelligente**. Invece di incollarti al telefono, tu dai al centralino l'elenco dei telefoni da tenere d'occhio, e poi ti metti a "dormire". Appena uno dei due telefoni squilla, il centralino ti sveglia e ti dice esattamente quale telefono devi alzare.

Ecco come questa logica si traduce riga per riga nel nostro ciclo `while`:

### **1. La "Lista da controllare" (`fd_set`)**
Il sistema operativo non conosce i nomi delle interfacce (`eth0`, `wlan0`), ma usa dei numeri chiamati **File Descriptor** (i nostri `fd_in` e `fd_out`).
Per dire alla `select` cosa ascoltare, usiamo una struttura chiamata `fd_set`, che è letteralmente un array di bit (una "lista delle spunte").

All'inizio di *ogni singolo ciclo*, dobbiamo preparare questa lista:
* `FD_ZERO(&readfds);` -> Prendi la lista e cancella tutto (togli tutte le spunte).
* `FD_SET(fd_in, &readfds);` -> Metti una spunta sul telefono 1.
* `FD_SET(fd_out, &readfds);` -> Metti una spunta sul telefono 2.

> **Nota importante:** Dobbiamo rifare questa operazione a ogni ciclo perché la `select`, quando ci sveglia, *modifica* questa lista per dirci chi ha suonato, cancellando le spunte da chi invece è rimasto muto.

### **2. Mettersi in "Attesa" (`select`)**
A questo punto chiamiamo il "centralino":
`int ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);`

Quando il programma esegue questa riga, **si ferma**. Il sistema operativo mette in "pausa" il nostro bridge. In questo stato, il programma consuma **0% di CPU**. Non stiamo facendo un ciclo continuo a vuoto chiedendo "C'è un pacchetto? C'è un pacchetto?". Stiamo letteralmente dormendo.

La `select` ci sveglierà e farà ripartire il codice solo se accade una di queste due cose:
1. **Arriva un pacchetto:** Uno dei due File Descriptor riceve dei dati.
2. **Scade il timeout (`tv`):** Nel nostro codice abbiamo impostato `tv.tv_sec = 1`. Significa: "Svegliami se arriva un pacchetto, ma se passa 1 secondo e non è arrivato nulla, svegliami lo stesso". Questo ci serve per far fare un giro a vuoto al ciclo `while` e permettergli di controllare se nel frattempo abbiamo premuto CTRL+C (`keep_running`).

### **3. Il Risveglio e l'Azione (`FD_ISSET`)**
Se la `select` si sblocca perché è arrivato un pacchetto, dobbiamo scoprire chi lo ha ricevuto. Come detto prima, la `select` ha modificato la nostra `readfds` lasciando la spunta solo sull'interfaccia che ha dati pronti.

Usiamo `FD_ISSET` per fare la domanda:
* `if (FD_ISSET(fd_in, &readfds))` -> "Centralino, è stato il telefono 1 a squillare?"
Se la risposta è sì, entriamo nell'`if`, usiamo `pcap_next_ex` per prendere il pacchetto (senza rischiare di bloccarci, perché siamo sicuri al 100% che il pacchetto c'è) e lo inoltriamo.
Facciamo poi la stessa domanda per l'interfaccia 2.

Finita questa fase, il ciclo `while` ricomincia: puliamo la lista, mettiamo le spunte, e andiamo di nuovo a dormire con la `select`.

---

È un meccanismo elegantissimo e molto efficiente, tipico della programmazione di rete e dei server ad alte prestazioni in C. 

Tutto chiaro il funzionamento del "centralino"? Vuoi che procediamo con l'ultimo step "bonus", ovvero implementare un filtro "manuale" a livello utente sui pacchetti per completare del tutto la consegna del prof, o preferisci fermarti qui e testare il codice?








Struttura base

while (bridge_attivo) {
    select(ascolta_su_FD1_e_FD2); // Aspetta finché non ci sono dati
    
    if (FD1_ha_dati) {
        pcap_next_ex(pd1, &header, &pacchetto);
        // [Eventuale logica di filtro manuale qui]
        pcap_sendpacket(pd2, pacchetto, header->caplen); // Invia sulla 2
        statistiche_dir1++;
    }
    
    if (FD2_ha_dati) {
        pcap_next_ex(pd2, &header, &pacchetto);
        // [Eventuale logica di filtro manuale qui]
        pcap_sendpacket(pd1, pacchetto, header->caplen); // Invia sulla 1
        statistiche_dir2++;
    }
}