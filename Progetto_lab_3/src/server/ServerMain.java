package server;

import common.ConfigReader;
import server.GameManager;
import server.WordDataset;
import server.NioServer;
import server.StorageManager;

import java.util.List;

//PUNTO DI INGRESSO PRINCIPALE PER IL SERVER

public class ServerMain{
    public static void main(String[] args){
        System.out.println("Avvio Server Connections...");
        
        //1. CARICO CONFIGURAZIONE
        ConfigReader config = new ConfigReader("config/server.properties");

        int port = config.getIntProperty("server.port", 8080);

        String datasetPath = config.getProperty("server.words.dataset.path", "data/words.json");

        long timerSeconds = config.getIntProperty("server.timer.seconds", 300); //default 5 minuti

        //Rimosso vecchio caricamento in blocco delle parole ora uso streaming API
        System.out.println("Dataset configurato: " + datasetPath);
        

        //3. INIZIALIZZO SERVER NIO
        NioServer server = new NioServer(port);
        

        //4. INIZIALIZZO IL GESTORE DEL GIOCO
        GameManager gameManager = new GameManager(datasetPath, timerSeconds, server);
        server.setGameManager(gameManager);
        
        
        // 5. Persistenza: carica utenti precedenti e avvia timer di salvataggio in background
        String dataDir = config.getProperty("server.data.dir", "data");
        int persistInterval = config.getIntProperty("server.persistence.interval.minutes", 5);
        
        StorageManager storage = new StorageManager(server.getRegisteredUsers(), dataDir + "/users.json", dataDir + "/games.json");
        storage.loadUsers();
        
        java.util.Map<Integer, GameManager.HistoricalGameStats> loadedGames = storage.loadPastGames();
        gameManager.importPastGames(loadedGames);
        storage.setPastGamesReference(gameManager.getPastGamesMap());
        
        storage.startPeriodicSave(persistInterval);
        
        // Aggiungiamo uno Shutdown Hook per intercettare CTRL+C (SIGINT) e salvare forzatamente i dati
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("\n[SERVER] Spegnimento rilevato. Salvataggio dati di emergenza in corso...");
            storage.stop();
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
