package server;

import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Map;

import server.NioServer;


//GESTISCE CICLO DI VITA DELLE PARTITE. CARICA DATASET, AVVIA TIMER E CAMBIA PARTITE
public class GameManager{
    
    private final String datasetFilePath; //Percorso del file JSON
    private int totalGamesCount = 0;

    //I thread worker in CommandProcessor accedono continuamente con .getCurrentGame quindi
    // devo fare in modo che sia thread safe
    private final AtomicReference<Game> currentGame = new AtomicReference<>();

    //Scheduler per il timer della partita
    private final ScheduledExecutorService scheduler;

    //Durata della singola partita
    private final long gameDurationSeconds; 

    //Riferimento al server per notifiche udp
    private final NioServer server;

    //Per partita casuale
    private final java.util.Random random = new java.util.Random();
    

    //Partita passata
    public static class HistoricalGameStats{
        public int gameId;
        public WordDataset.GameData data;
        public int totalParticipants;
        public int finishedParticipants;
        public int wonParticipants;
        public double averageScore;
    }

    //Insieme di tutte le partite passate
    private final Map<Integer, HistoricalGameStats> pastGames = new ConcurrentHashMap<>();



    public GameManager(String datasetFilePath, long durationSeconds, NioServer server){
        this.datasetFilePath = datasetFilePath;
        this.gameDurationSeconds = durationSeconds;
        this.server = server;
        this.scheduler = Executors.newSingleThreadScheduledExecutor();
    }


//---------------------------------------------------------------------------------------

    
    //AVVIA LA PRIMA PARTITA E IMPOSTA IL TIMER
    public void start(){

        this.totalGamesCount = WordDataset.countGames(datasetFilePath);
        System.out.println("[GAMEMANAGER] Trovate " + totalGamesCount + " partite nel file " + datasetFilePath);

        if(totalGamesCount <= 0){
            System.err.println("Dataset vuoto");
            return;
        }

        //Avvio la partita
        startNextGame();
    }


//---------------------------------------------------------------------------------------


    //CHIUDE LA PARTITA CORRENTE, NOTIFICA E AVVIA LA SUCCESSIVA
    private void startNextGame(){

        Game current = getCurrentGame();
        if(current != null){

            //Raccolgo le statistiche per la partita appena finita
            HistoricalGameStats stats = new HistoricalGameStats();
            stats.gameId = current.getGameId();
            stats.data = current.getData();
            int totalScore = 0;
            
            //Flusho il punteggio degli utenti che stavano giocando
            for(server.User user : server.getRegisteredUsers().values()){

                if(user.getCurrentGameId() != null && user.getCurrentGameId() == current.getGameId()){
                    stats.totalParticipants++;

                    //Ha completato una partita (vinta o persa)
                    if(user.isGameFinished()){
                        stats.finishedParticipants++;
                    }

                    //Ha vinto
                    if(user.getCurrentFoundGroups() != null && user.getCurrentFoundGroups().size() >= 3){
                        stats.wonParticipants++;
                    }
                    totalScore += user.getCurrentScore();
                    
                    //Imposto il game ID a -1
                    user.resetGameState(-1);
                }
            }

            //Punteggio medio
            if(stats.totalParticipants > 0){
                stats.averageScore = (double) totalScore / stats.totalParticipants;
            }

            //Metto la partita nello storico di game
            pastGames.put(stats.gameId, stats);
        }


        //Scelgo una nuova partita casuale, on demand tramite streaming api
        WordDataset.GameData data = null;
        int attemps = 0;
        
        //Faccio 10 tentativi se non ne trovo una nuova bona
        while(data == null && attemps < 10){

            int target = random.nextInt(totalGamesCount);
            //Passo il valore a loadGameAtIndex che mi carica un oggetto GameData
            WordDataset.GameData candidate = WordDataset.loadGameAtIndex(datasetFilePath, target);
            
            if(candidate != null){

                //Guardo se è già presente nello storico o se è il decimo tentativo
                if(!pastGames.containsKey(candidate.gameId) || attemps == 10){
                    data = candidate;
                }
            }
            attemps++;
        }
        
        //Creo la nuova istanza della partita e la imposto atomicamente
        Game newGame = new Game(data, gameDurationSeconds);
        currentGame.set(newGame);
        

        System.out.println("[GAMEMANAGER] Nuova partita casuale iniziata: ID " + newGame.getGameId() + ". Durata: " + gameDurationSeconds + "s");


        //Iscrivo automaticamente gli utenti loggati alla nuova partita e invio nuove parole
        server.broadcastNewGameStart(newGame);



        //Pianifico la fine della partita allo scadere del tempo
        scheduler.schedule(() ->{

            int endedGameId = currentGame.get().getGameId();
            System.out.println("[GAMEMANAGER] Tempo scaduto per la partita ID " + endedGameId);

            //Prima avvio la prossima partita per salvare le statistiche di quella conclusa in pastGames
            startNextGame();

            //Poi notifico i client (statistiche nello storico aggiornato per la stampa di reseconto)
            server.broadcastGameEnd(endedGameId);
        
        }, gameDurationSeconds, TimeUnit.SECONDS);
    }


//---------------------------------------------------------------------------------------

    
    //RESTITUISCE LA PARTITA ATTIVA
    public Game getCurrentGame(){
        return currentGame.get();
    }

    
//---------------------------------------------------------------------------------------


    //RESTITUISCE LE STATISTICHE PER UNA PARTITA CONCLUSA
    public HistoricalGameStats getHistoricalStats(int gameId){
        return pastGames.get(gameId);
    }

    
//---------------------------------------------------------------------------------------

    //RESTITUISCE RIFERIMENTO ALLA MAP DEI GIOCHI PASSATI
    public Map<Integer, HistoricalGameStats> getPastGamesMap(){
        return this.pastGames;
    }


//---------------------------------------------------------------------------------------


    //PRENDE LA MAPPA CARICATA DA FILE E LA CARICA IN MEMORIA
    public void importPastGames(Map<Integer, HistoricalGameStats> loadedGames) {
        if(loadedGames != null && !loadedGames.isEmpty()){
            this.pastGames.putAll(loadedGames);
            System.out.println("[GAMEMANAGER] Caricati " + loadedGames.size() + " giochi storici");
        }
    }
}
