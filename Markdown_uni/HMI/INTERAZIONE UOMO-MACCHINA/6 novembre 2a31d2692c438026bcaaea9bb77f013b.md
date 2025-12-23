# 6 novembre

# Design inclusivo

I prodotti digitali di oggi devono funzionare:

- **Per tutti**
- **Ovunque**: (dispositivi)
- **In modo costante**

Non sono problemi separati, sono interconnessi

## 3 Pilastri del design scalabile

- **Accessibilità**: Standards per design di interfacce che funzionano per tutti gli utenti, anche quelli con disabilità.
- **Design Responsive**: Interfacce che si adattano ad ogni tipo di device.
- **Design Systems**: Framework e libreria per mantenere consistenza tra prodotti e teams.

**Obiettivo**: Costruire interfacce che sono accessibili e responsive per design, mantenute costantemente attraverso sistemi

# 1. ACCESSIBILITY

## Perchè accessibilità?

- Il 15% della popolazione globale ha qualche forma di disabilità
- Disabilità temporanee affliggono tutti
- Requisiti legali in molti paesi

WCAG: Web Content Accessibily Guidlines

## Pour Principles

- **Percivable**: Informazione presentata in modi che l’utente possa percepire (alt text per immagini, cc per i video)
- **Operable**: Le componenti dell’interfaccia possano essere operativi per tutti gli utenti (keyboard access, tempo sufficiente, seizure triggers)
- **Understandable**: Informazione e operazioni siano chiare
- **Robust**: Contenuti funzionanti con tecnologie assistive

Conformance levels:

- Level A: Minimo
- Level AA: Baseline raccomandata (spesso target dei requirments legali)
- Level AAA: Avanzata

## Guidlines chiave e testing

Essential requirments (Livello AA)

- **Contrasti** per i colori
- **Keyboard access**
- **Alt text**
- **Semantic HTML**
- **Consistency**
- **Resizable text**

---

# 2. RESPONSIVE DESIGN

## Realtà multi device

Gli user accedono tramite schermi di diverse dimensioni e rapporti: Smartphones, Tablets, Laptops, Wearables ecc.

Il nostro design deve funzionare su tutto questo range.

## Cosa è un design responsive

<aside>
💡

**Definizione: Approccio design dove le interfacce adattano il loro layout, contenuti e funzionalità basati sulle caratteristiche del device, soprattutto dimensioni dello schermo**.

</aside>

Tecniche chiave:

- **Fluid grids**
- **Immagini flessibili**
- **Media queries**
- **Mobile-first thinking**

## Mobile-first approach

Perché pensare prima al mobile?

- Forza la prioritizzazione (spazio limitato = focus sulle cose essenziali)
- Più facile progressivamente aumentare che degradare
- L’uso mobile è quello maggiore
- Performance benefits (spesso mobile sta su connessioni più deboli=

<aside>
💡

**Progressive Enhancement**: Inizia con una funzionalità core, poi aggiunge migliorie per device più capaci.

</aside>

## Breakpoints Strategy

Consideri risoluzioni più comuni (1024p per tablet, 1280p per laptop ecc)

## Grid Systems

<aside>
💡

**Tecnica di layout che divide la pagina in colonne che gli elementi possono occupare**.

</aside>

Come funziona:

1. Divido in colonne uguali
2. Elementi usano 1-2 colonne
3. Lo spacing tra colonne da separazione visuale

![image.png](6%20novembre/image.png)

Es.

- Desktop: Contenuto principale usa 8 colonne, barra di lato usa 4 colonne
- Tablet: uguale
- Mobile: Entrambe usano 12 colonne, stack verticalmente

## 1. Responsive Patterns: TINY TWEAKS

(il nome del design è tiny tweaks)

 **Pattern più semplice**:

- Layout sta per la maggior parte uguale su tutti gli schermi
- Solo aggiustamenti minori
- Design a colonna singola

![image.png](6%20novembre/image%201.png)

**Quando** lo uso?

- Contenuti semplici e lineari
- Pagine single-purpose
- **Es**. Articoli semplici, form lineari

## 2. Responsive Patterns: COLUMN DROP

**Pattern più comune**:

- Inizia con una layout a colonna singola
- Aggiungi colonne all’aumentare della larghezza dello schermo
- Colonne cadono una sotto l’altra su schermi stretti

![image.png](6%20novembre/image%202.png)

**Quando** lo uso?

- Siti con tanti contenuti
- Layout multi-colonne su desktop
- **Es**. Siti di news, blog

## 3. Responsive Patterns: MOSTLY FLUID

**Variazione su Column Drop**:

- Grid diventa più fluida
- Margine aumenta su schermi più grandi
- Larghezza massima previene lunghezza della linea di diventare troppo grande

![image.png](6%20novembre/image%203.png)

**Quando** la uso?

- Applicazioni con aree contenuti definite
- Quando importa la leggibilità su schermi grandi
- Voglio esperienza consistente tra schermi
- **Es**. Email clients, app di produttività

## 4. Responsive Patterns: LAYOUT SHIFTER

**Più flessibile, più complesso**:

- Layout cambia significativamente ai breakpoints
- Diversi layout su diversi device
- Non solo column dropping, riorganizzazione fondamentale

![image.png](6%20novembre/image%204.png)

**Quando** si usa?

- Quando mobile e desktop devono avere esperienze differenti
- Applicazioni complesse

## 5. Responsive Patterns: OFF CANVAS

**Nasconde contenuto off-screen**

- Navigazione o contenuti secondari nascosti di default (su mobile)
- Mostrato tramite hamburger menù
- Sempre visibile su schermi larghi

![image.png](6%20novembre/image%205.png)

**Quando** si usa?

- App con navigazione estensiva
- Spazio limitato su mobile
- **Es**. Gmail, social media app

## Interazioni Touch vs Mouse

I dispositivi mobile introducono nuovi paradigmi di interazione

- Niente stati di “hover”
- Target di touch devono essere ingranditi
- Gestures
- Zone raggiungibili dal pollice

## Responsive Images & Media

Problema: Mandare immagini grandi da desktop a mobile spreca banda

Soluzioni:

- Attributo`srcset`: Dai tante dimensioni di immagini, il browser usa la migliore
- `<picture>`element: Diversi crop per dimensioni diverse
- `loading="lazy"`: Reindirizza off-screen caricamento delle immagini
- Immagini vettoriali

## Tecniche CSS

Tool di layout moderni:

- Flexbox: Layout 1-dimensione (colonne o righe) → [flexboxfroggy.com](https://flexboxfroggy.com)
- CSS Grid: Layout a 2 dimensioni (righe e colonne) → [cssgridgarden.com](https://cssgridgarden.com)
- Media queries: Aggiunge stili in base alla dimensione degli schermi

## Considerazioni di Performance

Per gli utenti mobile:

- Connessioni poco potenti
- Data plans limitati
- Processori meno potenti

Implicazione di design:

- Minimizzare richieste HTTP
- Ottimizzare immagini
- Lazy load

---

# 3. DESIGN SYSTEMS

## Problema di consistenza

Al crescere dei prodotti:

- Diversi designer fanno scelte diverse
- Componenti vengono rimpiazzati differentemente
- Inconsistenze si accumulano

## Cosa è un Design System?

<aside>
💡

**Una collazione di componenti riusabili, guidati da standard chiari, che possono essere assemblati per costruire applicazioni**.

</aside>

System design include:

- Libreria di componenti
- Design tokens
- Guidlines e documentazione
- Implementazioni di codice
- Ecc…

## Benefici di Design Systems

- **Consistency**: Stessi componenti si comportano uguale ovunque
- **Efficency**: Costruisci una volta, usa tante
- **Quality**: Accessibilità e responsività costruite insieme
- **Scalability**:
- **Maintenece**: Fix bug una volta, tutte le istanze ne beneficiano

## Metodologia

- **Atomi**: Blocchi di costruzione fondamentali (pulsanti, icone ecc)
- **Molecole**: Semplici combinazioni di atomi (Search form = input + button + ecc)
- **Organismi**: Componenti UI costruiti da molecole e atomi (Site header = logo + search form + ecc)
- **Templates**: Layout che mostrano struttura dei contenuti (Layout della homepage)
- **Pages**: Istanze specifiche di template

## Design Tokens

**Es**. 

![image.png](6%20novembre/image%206.png)

Perchè importano i tokens?

- Fonte singola di verità per decisioni di design
- Cambiano una volta, aggiorna ovunque

## Guide per Stile Front-End

Cosa c’è da documentare?

- Pattern di design visivi
- Componenti UI e loro variazioni
- Esempi di codice

A cosa servono?

- Aiutano nuovi membri
- Riducono domande frequenti
- Comprensione tra designer e sviluppatori

## Living Documentation

Best practice:

- Tenere documentazione insieme al codice
- Mostra esempi di componenti interattivi
- Snippet di codice
- Spiega i motivi di scelte

Tools: Storybook, Figma ecc

### Esempio: Material design

[**m3.material.io](https://m3.material.io) →Documentazione di google**

Ci sono altri esempi importante come **Apple Human Interface Guidlines**, **Carbon (IBM)** e altri.