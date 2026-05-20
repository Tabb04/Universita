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
        
    //---------------------------------------------------------------------------------------

        //1. CARICO CONFIGURAZIONE
        ConfigReader config = new ConfigReader("config/server.properties");

        int port = config.getIntProperty("server.port", 8080);

        String datasetPath = config.getProperty("server.wordspath", "data/words.json");

        long timerSeconds = config.getIntProperty("server.timer", 300); //default 5 minuti
        
        //Rimosso vecchio caricamento in blocco delle parole ora uso streaming API
        System.out.println("Dataset configurato: " + datasetPath);
        
    //---------------------------------------------------------------------------------------

        //3. INIZIALIZZO SERVER NIO
        NioServer server = new NioServer(port);
        
    //---------------------------------------------------------------------------------------

        //4. INIZIALIZZO IL GESTORE DEL GIOCO
        GameManager gameManager = new GameManager(datasetPath, timerSeconds, server);
        server.setGameManager(gameManager);
        
    //---------------------------------------------------------------------------------------
        
        // 5. LO STORAGE MANAGER: carica utenti precedenti e avvia timer di salvataggio in background ???
        String dataDir = config.getProperty("server.datapath", "data"); //Cartella con parole, utenti e partite passate
        int saveInterval = config.getIntProperty("server.saveinterval", 5);  //Intervallo di auto salvataggio
        
        //Passo il riferimento alla hashmap degli utenti e i file con utenti salvati e partite
        StorageManager storage = new StorageManager(server.getRegisteredUsers(), dataDir + "/users.json", dataDir + "/games.json");
        
        //Carico gli utenti
        storage.loadUsers();
        
        //Carico le partite precedenti e le metto in memoria
        java.util.Map<Integer, GameManager.HistoricalGameStats> loadedGames = storage.loadPastGames();
        gameManager.importPastGames(loadedGames);
        storage.setPastGamesReference(gameManager.getPastGamesMap());
        
        //Imposto loop di salvataggio
        storage.startPeriodicSave(saveInterval);
        

        //Catturo i SIGINT e salvo su file i dati
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("\n[SERVER] Rilevato Ctrl + c. Salvo dati su file.");
            storage.stop();
        }));

    //---------------------------------------------------------------------------------------

        //6. AVVIO GAMEMANAGER
        gameManager.start();

    //---------------------------------------------------------------------------------------

        //7. AVVIO SERVER IN ASCOLTO
        try{
            server.start();
        }catch(Exception e){
            e.printStackTrace();
        }
    }
}
