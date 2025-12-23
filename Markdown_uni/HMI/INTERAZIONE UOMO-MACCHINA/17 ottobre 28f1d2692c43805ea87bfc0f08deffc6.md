# 17 ottobre

# Human Error e Mitigation Strategies

Nella maggioranza dei casi un incidente ha una causa umana (75-95%).

Nella nostra cultura si tende a dare l’errore all’utente (persona).

La persona semplicemente porta a galla il problema del prodotto.

## Motivazioni

Si tende a progettare sistemi che pretendono che l’utente interagisca con essi al massimo dei livelli di attenzione.

Presupponiamo che il pensiero dell’utente permanga, mentre spesso l’utente sblocca automatismi (routine). 

Qui possono avvenire **DISTRAZIONI**. Una distrazione è un interruzione di un golfo.

Riprendere dopo una distrazione diventa molto più difficile e oneroso come carico mentale. Dopo una distrazione l’utente commette statisticamente più **errori**.

Quindi o **distrazioni** o **automatismi**.

**Es**. Iscrizione all’università. Dopo aver inserito CF e altri dati ripetute volte per errori di formattazione vari, all’arrivo alla parte importante sono già stanco mentalmente.

**Es**. Il flag per dire che l’indirizzo di fatturazione e spedizione sono uguali. Da come parte in ogni caso assolve i requisiti ma in uno da una migliore esperienza utente. Esprimibile tramite user story.

## Colpevole

Bisogna smettere di cercare il colpevole. Il colpevole è il designer. Dire che l’utente ha sbagliato non risolve nulla, se l’utente è incapace non ho soluzioni. Se la colpa è mia posso risolvere.

## Root Cause Analysis

Capire quale è stato l’insieme dei fattori che ha generato il problema.

**Es**. Incidente in auto nelle stesse condizioni può avere effetti molti diversi.
Questo perché le dinamiche degli incidenti sono così tante che un piccolo cambio può determinare tanto.

Gli incidenti avvengono per **concatenazione di cause**. 

Accettando che ci sono molte origini di problemi, devo provare a **disallinearle**. Oppure ridurre queste origini

Similitudine: Prendi fette di emmental e prova con una matita a perforarle tutte. Prendere tutti i buchi in linea è molto difficile.
Quindi per aiutare posso o disallineare i buchi o fare i buchi più piccoli.

![image.png](17%20ottobre/image.png)

**Es**. Come dire un numero di telefono o IBAN. L’iban è buona abitudine suddividere in parti utili.
Se mi sento dire il numero di telefono con aggruppamento diverso è difficile da capire. L’aggruppamento è il ridurre la fetta di formaggio.

## I 5 perché

Devo cambiare il paradigma di programmazione assodando che gli utenti sbagliano. Noi facciamo design per persone che errano, sono svogliate, distratte, non progettiamo per l’umano ideale.

Toyoda ha inventato questa regola.

<aside>
💡

**Non fermarsi al primo perché, farsene almeno 5.**

</aside>

I periti fanno questa cosa, e non si fermano a 5 di perché.

![image.png](17%20ottobre/image%201.png)

**Es**. Utente non ha inviato un certo dato. Perché? Non avevo segnalato che andava inserito? Perché non l’ha visto? Perché gli veniva segnalato solo in questo punto? …?

![image.png](17%20ottobre/image%202.png)

1. Oggettivare l’esperienza non farsi un opinione personale
2. Eliminare la sorgente di errore nel sistema non nell’utente
3. Crescere vedendo il miglioramento del proprio prodotto

Non si può risolvere un problema senza prima ammettere che il problema esiste.

## Nascita dell’errore

Nasce tra un disallineamento tra le capacità delle persone e come vengono realizzati i prodotti.

Nell’era dell’AI è ancora più necessario un interfaccia fatta bene.

Un errore è un qualsiasi discostamento dal comportamento atteso.

# Classificazione degli errori

Ci sono due classi principali di errori:

- **Slips** (lapsus)
- **Mistakes** (errori cognitivi)

![image.png](17%20ottobre/image%203.png)

## Slips

**Es**. Voglio prendere la macchina ed esco di casa con le chiavi della moto in mano.

Questi errori escono perché spesso siamo molto inconsci nelle cose che facciamo di routine.

### 1. Action Based

Pianifico bene l’azione ma la eseguo diversamente o in modo sbagliato.
**Es**. Metto il latte nel caffè e metto la tazza in frigo

### 2. Memory lapse

Non eseguo proprio l’azione.
**Es**. Lascio il gas acceso.

## Mistakes

Non ho realmente capito. Non si origina nella fase di azione, ma nella fase alta del golfo, nella pianificazione.

### 1. Rule Based

Sbaglio perché non conosco le regole del “gioco”.

### 2. Knowledge Based

Sbaglio perché il problema è diagnosticato male per conoscenza errata o incompleta.
**Es**. Disallineamento del modello mentale con il termostato. (esempio vecchio)
**Es**. Un meccanico che ti dice che la batteria è ancora buona ma si fulmina il giorno dopo.

### 3. Memory lapse

Dimenticanza agli stage del Goal e pianificazione.
**Es**. Meccanico non ha fatto troubleshooting corretto perché si è dimenticato uno step. 

I due Memory lapse sono diversi

1. Lapsus di memoria
2. Dimenticanza, lapsus tra le transizioni tra le due cose. Non ha origine nell’azione ma è nell’aver interrotto o saltato una parte della pianificazione.

![image.png](17%20ottobre/image%204.png)

Uno è un errore nelle fasi, uno nelle transizioni delle fase.

## Interruzioni

Portano a memory-lapse. Dopo un interruzione è probabile anche dover ricominciare da capo.

## Feedback sbagliati

I feedback eccessivi rovinano l’esperienza dell’utente esperto, perché aver ignorato quelli inutili si ignorano anche quelli effettivamente utili.

I Feedback vocali sono ormai ritenuti disutili e fastidiosi.
**Es**. Distributore delle sigarette che parla sempre.

Inoltre un voiceover mentre si legge può disturbare la lettura.

# Prevenzione di errori

Bisogna fare:

- **Sensibility check**: 
Es. Faccio sempre bonifici da 500 euro, oggi ne fai uno da 10.000. Mi interrompo e ti chiedo una conferma via mail.
- **Possibilità di fare undo**
- **Rendere facile per l’utente capire l’errore:**
Es. Evidenziare di rosso il campo sbagliato in un inserimento dati e dire quale è l’errore (formattazione, caratteri ecc.)
- **Non trattare le azioni dell’utente come errore**

## Aggiungere constraints per bloccare errori

Più un sistema è vincolato meno errori provoca.

Separare i moduli inoltre aiuta molto nel capire il software. Mescolo mapping e constrains e si chiamano **controlli segregati**.

## Messaggi di conferma

Se in un software che non chiede quasi mai una conferma, quando arriva me ne accorgo. Il contrario no.

Anche i lock in perdono significato, da quanti ce ne sono.

Se possibile io salvo sempre anche se l’utente non lo ha chiesto.

**Es**. Google doc non ti fa creare un documento vuoto, devi per forza dare un nome. Su word quando apro un documento fino a quando non lo salvo è nel limbo (almeno prima era così).

## Minimizzare slip

Sapendo che l’utente farà sempre errori, devo provare a minimizzare gli incidenti.
**Es**. Se l’utente non salva il file, io lo salvo sui miei server comunque

# Principi chiave

- Mi assicuro che ho dato all’utente tutta la conoscenza per creare un modello mentale che si vicino a quello mentale
- Usare feedback e mapping ecc. per aiutare l’utente nel comportamento inconscio
- Lavorare per colmare il più possibile la distanza tra i due golfi