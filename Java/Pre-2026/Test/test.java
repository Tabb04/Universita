public class CalendarioTurni {
    // --- PARTE 1: CREAZIONE SINGLETON (THREAD-SAFE PER DEFINIZIONE) ---

    // Eager initialization: La JVM crea l'istanza al caricamento della classe.
    // 'final' assicura che il riferimento non cambi mai.
    // Non serve 'synchronized' qui o su getInstance().
    private static final CalendarioTurni instance = new CalendarioTurni();

    // Stato interno dell'oggetto (i dati condivisi)
    private String[] turni;

    // Costruttore privato
    private CalendarioTurni() {
        turni = new String[0]; // Inizializza array vuoto
    }

    // Metodo di accesso all'istanza
    // Essendo eager, questo metodo è velocissimo e non richiede lock.
    public static CalendarioTurni getInstance() {
        return instance;
    }

    // --- PARTE 2: GESTIONE DATI (THREAD-SAFE ESPLICITA) ---

    // Aggiungiamo 'synchronized' per proteggere i dati.
    // Se un thread sta leggendo, nessun altro può scrivere finché non ha finito.
    public synchronized String[] getTurni() {
        return turni;
    }

    // Aggiungiamo 'synchronized' per evitare che due thread scrivano insieme
    // sovrascrivendosi a vicenda (Race Condition).
    public synchronized void modificaCalendario(String[] nuoviTurni) {
        this.turni = nuoviTurni;
    }
}