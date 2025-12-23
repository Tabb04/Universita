# 28 novembre

# Ricerca quantitativa

# 1. COSA È LA RICERCA QUANTITATIVA

Ricerca quantitativa coinvolge collezionare e analizzare dati numerici per:

- Misurare variabili
- Testare ipotesi
- Identificare pattern

**Caratteristiche chiave:**

- Strutturato, approccio predeterminato
- Data collection standardizzato
- Sample size molto grandi

<aside>
💡

Da ricerca qualitativa (comprendere) a larghezza quantitativa (misurazione).

</aside>

## Quando usare la ricerca quantitativa

Ideale per:

- Testare ipotesi specifiche
- Misurare performance
- Comparare alternative
- Generalizzare la popolazione

Esempi in HCI:

- “La Dark mode riduce stress sugli occhi?”
- Che percentuale di user completano ondboarding?”
- Quale organizzazione della UI permette completamento task più rapido?”

## Da Qualitativo a Quantitativo

**Qualitativo**:

- Esplora significati, esperienze
- Open-ended flessibile
- Perché? Come? Cosa significa?

**Quantitativo**:

- Misurare variabili
- Strutturato, predeterminato
- Quanti? Quanto? Quanto spesso?

## Due metodi quantitativi

**Surveys vs Experiments**

**Questionario**:

- Composizione:
    - Colleziono dati self-reported
    - Sample grandi possibili
    - Questionari strutturati
- Forze:
    - Cost-effective
    - Raggiunge tante persone
- Limitazioni:
    - Self-report bias
    - Non si può stabilire causalità

**Esperimenti**:

- Composizione
    - Comparazioni controllate
    - Variabili manipolate
    - Stabilisco causalità
- Forze:
    - Controllo sulle variabili
    - Posso capire causalità
- Limitazioni:
    - Settings artificiali
    - Time-intensive
    - Sample più piccoli

# 2. QUESTIONARI

## Quando utilizzo questionari?

Aiutano a capire:

- Opinioni e attitudini
- Comportamenti self-reported
- Consapevolezza

Tipi di surveys:

- **Descrittivi**: Come è la distribuzione? Quanto è comune?
- **Analitici**: Che relazioni esistono? Cosa predicono?

Non buoni per:

- Misurazioni di performance oggettive
- Stabilire causalità

## Pianificare uno studio questionario

1. **Definire il focus della ricerca**
2. **Scegliere il questionario**
3. **Pianificare il sampling**
4. **Test pilota**
5. **Colleziona e analizza**

## Scrivere le domande

**Caratteristiche:**

- Chiare e non ambigue
- Neutrali
- Singole (una sola idea per domanda)

**Possono essere:**

- Domande **aperte** (poco usate)
- Domande **chiuse**: Si o No, Scale a punti dispari solitamente

**Scale di Likert:**

- 5 o 7 punti
- 1=Strongly Disagree, 2=Disagree, 3=Neutral, 4=Agree, 5=Strongly Agree

## Sampling per i surveys

**Terminologia:**

- Popolazione: Tutti i potenziali user
- Sampling frame: Quelli contattabili
- Sample: Chi ho invitato
- Respondents: Chi completa il survey

**Sampling strategies:**

- Random: Tutti i membri hanno le stesse chance
- Stratificato: Assicura una rappresentazione dei sottogruppi
- Convenienza: Usare persone disponibili
- Snowball: Partecipanti coinvolgono altri

Sample size: HCI survey avarage ~371 respondents, ma variano molto

## Respose rate e bias

Response rates spesso sono bassi:

- Online: 10-30%
- Email: 20-40%
- In persona: Alto

Non-response bias: Quelli che rispondono potrebbero differire da quelli che non lo fanno (ad esempio opinioni politiche)

Migliorare response rate: 

- Survey bassi
- Spiegare bene
- Incentivi
- Reminders

# 3. DESIGN SPERIMENTALE

## Perché esperimenti?

Permettono di:

- Comparare alternative
- Controllare fattori esterni
- Misurazioni precise
- Capire Interazioni

Quanto usare esperimenti?

- Testare ipotesi specifiche
- Misurare performance
- Valutare algoritmi

<aside>
💡

**Esperimenti danno comparazioni controllate e possono stabilire causalità**.

</aside>

## Variabili

**Variabili Indipendenti (IV)**

- Descrizione:
    - Cosa posso manipolare
    - La “causa”
    - Sotto controllo
- Esempi:
    - Metodi di input (touch, mouse)
    - Layout di interfacce
- Livelli (Diversi valori di IV)
    - Mouse vs Touchpad vs Trackpoint

**Variabili Dipendenti (DV)**

- Descrizione:
    - Cosa posso misurare
    - L’”effetto”
    - Outcome di interesse
- Esempi:
    - Task completion time
    - Error rate
    - Rating di soddisfazione
    - Click per completare un task
- Dovrebbe essere:
    - Oggettiva
    - Affidabile
    - Rilevante

<aside>
💡

Ipotesi: IV ha effetto su DV: 
**Es** “Input touch sarà più veloce di input mouse”.

</aside>

## Stile di esperimento

Between subject vs Within subject.

**Between-subject**: 

- Descrizione:
    - Ogni partecipante sottoposto ad una sola condizione
    - Gruppi diversi per ogni condizione
- Vantaggi:
    - No learning effects
    - No fatica
    - Analisi facile
- Svantaggi
    - Bisogno di tanti partecipanti
    - Differenze individuali importanti
- Es.
    - Gruppo A usa interfaccia 1, Gruppo B Interfaccia 2.

**Within-subject**:

- Descrizione
    - Ogni partecipante ha esperienza di tutte le condizioni
    - Stesse persone
- Vantaggi:
    - Meno partecipanti necessari
- Svantaggi:
    - Learning effects
- Es.
    - Tutti usano sia interfaccia 1 che 2.

## Cofounds e controlli

**Confouding variables**: Qualcosa che ha influenza sulle DV ma non è un IV

Esempio:

- Esperienze a priori
- Tempo del test
- Ambiente

Come controllare confounds: 

- Randomizzazione
- Counterbalancing
- Standardizzazione
- Controllo statistico

## Ipotesi: Fare predizioni

Ipotesi: Predizioni testabili riguardo le relazioni tra variabili.

Due tipi:

- Ipotesi nulle ($H_0$): No effetto, no differenze
- Ipotesi alternativa ($H_1$): C’è un effetto

Esempio:

- $H_0$: Non c’è una differenza nel completamento task tra touch e input mouse
- $H_1$: Tempo di completamento task cambia

## Dimensione del sample e potenza

Analisi della potenza: aiuta a determinare la sample size basandosi su differenza attesa, potenza statistica, livelli significativi ecc..

Studi HCI tipici:

- 12 partecipanti in media
- 20 per esperimenti controllati

<aside>
💡

Per medium effect size, 80% potenza: necessito 64 partecipanti per gruppo

</aside>

# 4. ANALISI STATISTICA

## Tipi di statistica

- **Statistica descrittiva**: Riassumere dati (media, mediana, deviazione standard ecc)
- **Statistica inferenziale**: Dare conclusioni sulla popolazione

### Statistica descrittiva

- **Misure delle tendenze** (Media, Mediana, Moda)
- **Misure della diffusione** (Deviazione standard, Range, Range Interquartile)
- **Visualizzazioni** (Istogrammi, Box-plots, Bar Charts)

## Testing delle ipotesi: Logica

1. Assumo ipotesi nulla è vera
2. Calcolo: Quanto è probabile che i risultati siano così estremi **se $H_0$ è vera**?
3. Se molto poco probabile (p<0.05), rifiuto $H_0$
4. Se non è abbastanza improbabile, fallisco a rifiutare $H_0$

<aside>
💡

P-value = probabilità di avere risultati così estremi se l’ipotesi nulla è vera.

</aside>

Livello di Significanza ($\alpha$):

- Threshold comune: $p$  < 0.05
- Means: <5% delle probabilità dei risultati è per randomicità

<aside>
💡

$p$ < 0.05 non vuol dire “provato”, significa “improbabile sia causale”

</aside>

# 5. QUALITÀ E ETICA

## Validità negli esperimenti

1. **Validità interna**: La IV influenza davvero la DV?
2. **Validità esterna**: I risultati generalizzano?

## Pericoli comuni per la validità

- **Selection Bias**
- **Demand characteristic**
- **Experimenter affects**
- **Fatigue effects**
- **Practice effects**

## Considerazioni etiche

- Consenso informale
- No danni
- Privacy
- Compensazione onesta