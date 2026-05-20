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


//GESTISCE IL CARICAMENTO E SALVATAGGIO DEI DATI DEGLI UTENTI SU FILE 
public class StorageManager{
    private final ConcurrentHashMap<String, User> registeredUsers;  //Riferimento agli utenti registrati
    private final String usersFilePath;
    private Map<Integer, HistoricalGameStats> pastGames;
    private final String gamesFilePath;
    private final ScheduledExecutorService scheduler;
    private final Gson gson;

    public StorageManager(ConcurrentHashMap<String, User> registeredUsers, String usersFilePath, String gamesFilePath){
        this.registeredUsers = registeredUsers;
        this.usersFilePath = usersFilePath;
        this.gamesFilePath = gamesFilePath;

        //Uso setPrettyPrinting per formattarlo ammodo
        this.gson = new GsonBuilder().setPrettyPrinting().create();
        
        //Per la chiamata periodica
        this.scheduler = Executors.newSingleThreadScheduledExecutor();
    }

    
//---------------------------------------------------------------------------------------


    //RIFERIMENTO ALL'INSIEME DI PARTITE PASSATE
    public void setPastGamesReference(Map<Integer, HistoricalGameStats> pastGames){
        this.pastGames = pastGames;
    }


//---------------------------------------------------------------------------------------


    //CARICA GLI UTENTI DA FILE
    public void loadUsers(){

        File file = new File(usersFilePath);
        if(!file.exists()){
            System.out.println("[STORAGE] Nessun file degli utenti trovato.");
            return;
        }

        try(FileReader reader = new FileReader(file)){

            //Per problema dei generici a tempo di compilazione
            Type type = new TypeToken<ConcurrentHashMap<String, User>>(){}.getType();
            ConcurrentHashMap<String, User> loaded = gson.fromJson(reader, type);

            if(loaded != null){

                //Inserisco tutti negli utenti registrati
                registeredUsers.putAll(loaded);
                System.out.println("[STORAGE] Caricati " + loaded.size() + " utenti registrati da file.");
            }

        }catch(IOException e){
            System.err.println("[STORAGE] Errore durante il caricamento degli utenti. " + e.getMessage());

        }
    }


//---------------------------------------------------------------------------------------
  

    //CARICA LE PARTITE PRECEDENTI DA FILE
    public Map<Integer, HistoricalGameStats> loadPastGames(){

        File file = new File(gamesFilePath);

        //Se non c'è il file non carico nulla
        if(!file.exists()){
            return new ConcurrentHashMap<>();
        }

        try (FileReader reader = new FileReader(file)){

            Type type = new TypeToken<ConcurrentHashMap<Integer, HistoricalGameStats>>(){}.getType();
            Map<Integer, HistoricalGameStats> loaded = gson.fromJson(reader, type);

            if(loaded != null){
                System.out.println("[STORAGE] Caricate " + loaded.size() + " partite concluse da file.");
                return loaded;
            }

        }catch(IOException e){
            System.err.println("[STORAGE] Errore durante il caricamento delle partite. " + e.getMessage());
        }

        return new ConcurrentHashMap<>();
    }


//---------------------------------------------------------------------------------------


    //AVVIA UN THREAD SCHEDULED CHE PERIODICAMENTE SALVA
    public void startPeriodicSave(int intervalMinutes){
        System.out.println("[STORAGE] Salvataggio automatico impostato ogni " + intervalMinutes + " minuti.");
        scheduler.scheduleAtFixedRate(() -> this.saveAll(), intervalMinutes, intervalMinutes, TimeUnit.MINUTES);
    }
    

    //FUNZIONE DA CHIAMARE PER IL SALVATAGGIO
    public void saveAll(){
        saveUsers();
        savePastGames();
    }


    //SCRITTURA DELLA MAPPA UTENTI SU FILE
    public void saveUsers(){

        File file = new File(usersFilePath);

        //Sovrascrive tutto quello su file 
        try(FileWriter writer = new FileWriter(file)){

            //Scrivo gli utenti registrati su file (ignora i dati transient sulla partita corrente)
            gson.toJson(registeredUsers, writer);
            System.out.println("[STORAGE] Utenti salvati su file.");

        }catch(IOException e){
            System.err.println("[STORAGE] Fallito il salvataggio utenti: " + e.getMessage());            
        }
    }
    

    //SCRITTURA DELLO STORICO PARTITE SU FILE
    public void savePastGames(){

        if(pastGames == null){
            return;
        }

        File file = new File(gamesFilePath);
        
        try(FileWriter writer = new FileWriter(file)){
            gson.toJson(pastGames, writer);
            System.out.println("[STORAGE] Partite salvate con successo su disco.");

        }catch(IOException e){
            System.err.println("[STORAGE] Fallito il salvataggio partite: " + e.getMessage());
        }
    }
    

//---------------------------------------------------------------------------------------


    //CHIAMATA ALLA CHIUSURA DEL SERVER PER ULTIMO SALVATAGGIO
    public void stop(){
        saveAll(); 
        scheduler.shutdown();
    }
}
