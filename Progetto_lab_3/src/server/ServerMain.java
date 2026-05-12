package server;

import common.ConfigReader;
import server.GameManager;
import server.WordDataset;
import server.NioServer;
import server.PersistenceManager;

import java.util.List;

/**
 * Punto di ingresso principale per il Server.
 */
public class ServerMain {
    public static void main(String[] args) {
        System.out.println("Avvio Server Connections...");
        
        // 1. Carica configurazione
        ConfigReader config = new ConfigReader("config/server.properties");
        int port = config.getIntProperty("server.port", 8080);
        String datasetPath = config.getProperty("server.words.dataset.path", "data/words.json");
        long timerSeconds = config.getIntProperty("server.timer.seconds", 300); // default 5 minuti
        
        // 2. Carica dataset delle parole
        System.out.println("Caricamento dataset: " + datasetPath);
        List<WordDataset.GameData> dataset = WordDataset.loadDataset(datasetPath);
        if (dataset == null || dataset.isEmpty()) {
            System.err.println("Impossibile caricare il dataset. Chiusura.");
            return;
        }
        
        // 3. Inizializza Rete (Server NIO)
        NioServer server = new NioServer(port);
        
        // 4. Inizializza Gestore Gioco (GameManager)
        GameManager gameManager = new GameManager(dataset, timerSeconds, server);
        server.setGameManager(gameManager);
        
        // 5. Persistenza: carica utenti precedenti e avvia timer di salvataggio in background
        String dataDir = config.getProperty("server.data.dir", "data");
        int persistInterval = config.getIntProperty("server.persistence.interval.minutes", 5);
        PersistenceManager persistence = new PersistenceManager(server.getRegisteredUsers(), dataDir + "/users.json", dataDir + "/games.json");
        persistence.loadUsers();
        
        java.util.Map<Integer, GameManager.HistoricalGameStats> loadedGames = persistence.loadPastGames();
        gameManager.loadPastGames(loadedGames);
        persistence.setPastGamesReference(gameManager.getPastGamesMap());
        
        persistence.startPeriodicSave(persistInterval);
        
        // Aggiungiamo uno Shutdown Hook per intercettare CTRL+C (SIGINT) e salvare forzatamente i dati
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("\n[SERVER] Spegnimento rilevato. Salvataggio dati di emergenza in corso...");
            persistence.stop();
        }));
        
        // 6. Avvia GameManager (timer prima partita)
        gameManager.start();
        
        // 7. Avvia Server NIO (Metodo bloccante, starà in ascolto)
        try {
            server.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
