package server;

import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Map;

import server.NioServer;

/**
 * Gestisce il ciclo di vita delle partite di Connections.
 * Si occupa di caricare il dataset, avviare il timer e ruotare la partita
 * allo scadere del tempo, oltre a mantenere uno storico statistico.
 */
public class GameManager {
    private final List<WordDataset.GameData> allGames; // Tutte le partite lette dal JSON
    private int currentGameIndex = 0; // Indice della partita attuale
    private final AtomicReference<Game> currentGame = new AtomicReference<>(); // Riferimento thread-safe alla partita attiva
    private final ScheduledExecutorService scheduler; // Scheduler per il timer della partita
    private final long gameDurationSeconds; // Durata della singola partita
    private final NioServer server; // Riferimento al server per notificare i client (UDP)
    
    // Storico delle statistiche per le partite passate
    public static class HistoricalGameStats {
        public int gameId;
        public WordDataset.GameData data;
        public int totalParticipants;
        public int finishedParticipants;
        public int wonParticipants;
        public double averageScore;
    }
    private final Map<Integer, HistoricalGameStats> pastGames = new ConcurrentHashMap<>();

    public GameManager(List<WordDataset.GameData> dataset, long durationSeconds, NioServer server) {
        this.allGames = dataset;
        this.gameDurationSeconds = durationSeconds;
        this.server = server;
        this.scheduler = Executors.newSingleThreadScheduledExecutor();
    }

    /**
     * Avvia la prima partita e imposta il timer ricorrente.
     */
    public void start() {
        if (allGames == null || allGames.isEmpty()) {
            System.err.println("Dataset vuoto, impossibile avviare GameManager.");
            return;
        }
        startNextGame();
    }

    /**
     * Chiude la partita corrente, notifica i giocatori e avvia la successiva.
     */
    private void startNextGame() {
        Game current = getCurrentGame();
        if (current != null) {
            // Raccogliamo le statistiche per la partita appena conclusa
            HistoricalGameStats stats = new HistoricalGameStats();
            stats.gameId = current.getGameId();
            stats.data = current.getData();
            int totalScore = 0;
            
            // Prima di cambiare partita, cristallizziamo (flush) il punteggio transiente degli utenti che stavano giocando
            for (server.User user : server.getRegisteredUsers().values()) {
                if (user.getCurrentGameId() != null && user.getCurrentGameId() == current.getGameId()) {
                    stats.totalParticipants++;
                    if (user.isGameFinished()) {
                        stats.finishedParticipants++;
                    }
                    if (user.getCurrentFoundGroups() != null && user.getCurrentFoundGroups().size() >= 3) {
                        stats.wonParticipants++;
                    }
                    totalScore += user.getCurrentScore();
                    
                    user.resetGameState(-1); // Sposta lo score e disabilita il transiente
                }
            }
            if (stats.totalParticipants > 0) {
                stats.averageScore = (double) totalScore / stats.totalParticipants;
            }
            pastGames.put(stats.gameId, stats);
        }

        // Estraiamo i dati per la nuova partita
        WordDataset.GameData data = allGames.get(currentGameIndex);
        
        // Creiamo la nuova istanza della partita e la impostiamo atomicamente
        Game newGame = new Game(data, gameDurationSeconds);
        currentGame.set(newGame);
        
        System.out.println("[GAMEMANAGER] Nuova partita iniziata: ID " + newGame.getGameId() + ". Durata: " + gameDurationSeconds + "s");

        // Prepariamo l'indice per la partita successiva
        currentGameIndex = (currentGameIndex + 1) % allGames.size();

        // Pianifichiamo la fine della partita allo scadere del tempo
        scheduler.schedule(() -> {
            System.out.println("[GAMEMANAGER] Tempo scaduto per la partita ID " + currentGame.get().getGameId());
            // Chiudiamo le sessioni per i giocatori e inviamo un messaggio di fine partita
            if (server != null) {
                server.broadcastGameEnd(currentGame.get().getGameId());
            }
            // Iniziamo la partita successiva
            startNextGame();
        }, gameDurationSeconds, TimeUnit.SECONDS);
    }

    /**
     * Ritorna la partita correntemente attiva.
     * @return Game attuale
     */
    public Game getCurrentGame() {
        return currentGame.get();
    }
    
    /**
     * Ritorna le statistiche storiche per una partita conclusa.
     * @param gameId ID della partita
     * @return Statistiche storiche o null se non trovata
     */
    public HistoricalGameStats getHistoricalStats(int gameId) {
        return pastGames.get(gameId);
    }
    
    public Map<Integer, HistoricalGameStats> getPastGamesMap() {
        return this.pastGames;
    }

    public void loadPastGames(Map<Integer, HistoricalGameStats> loadedGames) {
        if (loadedGames != null && !loadedGames.isEmpty()) {
            this.pastGames.putAll(loadedGames);
            
            // Cerca il gameId più alto nello storico per riprendere il giro
            int maxId = -1;
            for (Integer id : loadedGames.keySet()) {
                if (id > maxId) maxId = id;
            }
            
            if (maxId >= 0 && maxId < allGames.size() - 1) {
                this.currentGameIndex = maxId + 1;
                System.out.println("[GAMEMANAGER] Ripresa dalla partita index: " + currentGameIndex);
            } else if (maxId >= allGames.size() - 1) {
                this.currentGameIndex = 0; // Se le ha fatte tutte, ricomincia da zero
                System.out.println("[GAMEMANAGER] Tutte le partite concluse, si ricomincia dall'inizio.");
            }
        }
    }
}
