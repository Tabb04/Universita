package server;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;
import server.User;
import server.GameManager.HistoricalGameStats;

import java.io.*;
import java.lang.reflect.Type;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Gestisce il salvataggio (persistenza) e il caricamento dei dati degli utenti
 * su file JSON. L'operazione di salvataggio avviene in background in modo periodico.
 */
public class PersistenceManager {
    private final ConcurrentHashMap<String, User> registeredUsers;
    private final String usersFilePath;
    private Map<Integer, HistoricalGameStats> pastGames;
    private final String gamesFilePath;
    private final ScheduledExecutorService scheduler;
    private final Gson gson;

    public PersistenceManager(ConcurrentHashMap<String, User> registeredUsers, String usersFilePath, String gamesFilePath) {
        this.registeredUsers = registeredUsers;
        this.usersFilePath = usersFilePath;
        this.gamesFilePath = gamesFilePath;
        // Gson configurato in modalità PrettyPrinting per rendere il file JSON leggibile dall'uomo
        this.gson = new GsonBuilder().setPrettyPrinting().create();
        this.scheduler = Executors.newSingleThreadScheduledExecutor();
    }
    
    public void setPastGamesReference(Map<Integer, HistoricalGameStats> pastGames) {
        this.pastGames = pastGames;
    }

    /**
     * Carica gli utenti dal file al riavvio del server.
     */
    public void loadUsers() {
        File file = new File(usersFilePath);
        if (!file.exists()) {
            System.out.println("[PERSISTENCE] Nessun file utenti trovato (partiamo da zero).");
            return;
        }

        try (FileReader reader = new FileReader(file)) {
            Type type = new TypeToken<ConcurrentHashMap<String, User>>(){}.getType();
            ConcurrentHashMap<String, User> loaded = gson.fromJson(reader, type);
            if (loaded != null) {
                registeredUsers.putAll(loaded);
                System.out.println("[PERSISTENCE] Caricati " + loaded.size() + " utenti registrati.");
            }
        } catch (IOException e) {
            System.err.println("[PERSISTENCE] Errore durante il caricamento degli utenti: " + e.getMessage());
        }
    }

    /**
     * Carica lo storico partite dal file.
     */
    public Map<Integer, HistoricalGameStats> loadPastGames() {
        File file = new File(gamesFilePath);
        if (!file.exists()) {
            return new ConcurrentHashMap<>();
        }
        try (FileReader reader = new FileReader(file)) {
            Type type = new TypeToken<ConcurrentHashMap<Integer, HistoricalGameStats>>(){}.getType();
            Map<Integer, HistoricalGameStats> loaded = gson.fromJson(reader, type);
            if (loaded != null) {
                System.out.println("[PERSISTENCE] Caricate " + loaded.size() + " statistiche di partite concluse.");
                return loaded;
            }
        } catch (IOException e) {
            System.err.println("[PERSISTENCE] Errore durante il caricamento delle partite: " + e.getMessage());
        }
        return new ConcurrentHashMap<>();
    }

    /**
     * Avvia un thread schedulato per eseguire il salvataggio periodico.
     */
    public void startPeriodicSave(int intervalMinutes) {
        System.out.println("[PERSISTENCE] Salvataggio automatico impostato ogni " + intervalMinutes + " minuti.");
        scheduler.scheduleAtFixedRate(this::saveAll, intervalMinutes, intervalMinutes, TimeUnit.MINUTES);
    }
    
    public void saveAll() {
        saveUsers();
        savePastGames();
    }

    /**
     * Esegue la scrittura della mappa utenti nel file JSON.
     */
    public void saveUsers() {
        File file = new File(usersFilePath);
        file.getParentFile().mkdirs(); // Crea la cartella /data se non esiste

        try (FileWriter writer = new FileWriter(file)) {
            // Gson serializza la mappa tranquillamente, ed eviterà
            // di salvare le variabili 'transient' in User (es. la partita in corso)
            gson.toJson(registeredUsers, writer);
            System.out.println("[PERSISTENCE] Utenti salvati con successo su disco.");
        } catch (IOException e) {
            System.err.println("[PERSISTENCE] Fallito il salvataggio utenti: " + e.getMessage());
        }
    }
    
    /**
     * Esegue la scrittura dello storico partite nel file JSON.
     */
    public void savePastGames() {
        if (pastGames == null) return;
        File file = new File(gamesFilePath);
        file.getParentFile().mkdirs(); // Crea la cartella /data se non esiste
        
        try (FileWriter writer = new FileWriter(file)) {
            gson.toJson(pastGames, writer);
            System.out.println("[PERSISTENCE] Partite storiche salvate con successo su disco.");
        } catch (IOException e) {
            System.err.println("[PERSISTENCE] Fallito il salvataggio partite: " + e.getMessage());
        }
    }
    
    /**
     * Da invocare prima della chiusura totale del server per salvare l'ultima volta.
     */
    public void stop() {
        saveAll(); 
        scheduler.shutdown();
    }
}
