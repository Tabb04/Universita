package server;

import java.util.*;

public class User{
    private String username;
    private String password;
    
    
    private int gamesPlayed = 0;
    private int gamesWon = 0;
    private int globalScore = 0;
    

    private int puzzlesCompleted = 0;
    private int puzzlesWon = 0;
    private int puzzlesLost = 0;
    private int currentStreak = 0;
    private int maxStreak = 0;
    private int perfectPuzzles = 0;     //Con 0 errori

    
    /*Ho strutturato array come:
        Posizione [0-3]: Partite vinte con i errori
        Posizione [4]: Partite perse con 4 errori
        Posizione [5]: Partite perse per tempo
    */
    private int[] mistakeHistogram = new int[6];
    
    //Storico dei risultati delle partite precedenti dell'utente
    public static class GameResult{
        public int score;
        public int mistakes;
        public boolean won;

        public GameResult(int score, int mistakes, boolean won){
            this.score = score;
            this.mistakes = mistakes;
            this.won = won;
        }
    }

    private Map<Integer, GameResult> history = new HashMap<>();


    public User(String username, String password) {
        this.username = username;
        this.password = password;
    }


    //Stato transient del il gioco in corso, non salvato su JSON
    private transient Integer currentGameId = -1;
    private transient Set<String> currentFoundGroups = new HashSet<>();     //Temi al momento indovinati dall'utente
    private transient int currentMistakes = 0;
    private transient int currentScore = 0;
    private transient boolean gameFinished = false; //per questo utente
    

    public String getUsername(){
        return username;
    }

    public void setUsername(String username){
        this.username = username;
    }
    
    public String getPassword(){
        return password;
    }

    public void setPassword(String password){
        this.password = password;
    }
    
    public int getGamesPlayed(){
        return gamesPlayed;
    }
    
    public int getGamesWon(){
        return gamesWon;
    }

    public int getGlobalScore(){
        return globalScore;
    }


    //Invocata quando scade il timer del server e passo alla partita successiva
    public synchronized void resetGameState(int gameId){

        //Prima di resettare trasferisco il risultato nelle statistiche
        if((this.currentGameId != null) && (this.currentGameId != -1)){
            this.gamesPlayed++;
            this.globalScore += this.currentScore;
            boolean won = (this.currentFoundGroups != null) && (this.currentFoundGroups.size() == 4);
            
            //Salvo nello storico delle partite per l'istogramma
            this.history.put(this.currentGameId, new GameResult(this.currentScore, this.currentMistakes, won));
            
            this.puzzlesCompleted++;
            
            if(won){
                this.gamesWon++;
                this.puzzlesWon++;
                this.currentStreak++;

                if(this.currentStreak > this.maxStreak){
                    this.maxStreak = this.currentStreak;
                }

                if(this.currentMistakes == 0){
                    this.perfectPuzzles++;
                }

                
                if(this.currentMistakes >= 0 && this.currentMistakes < 4){
                    this.mistakeHistogram[this.currentMistakes]++;      //Aumento il relativo punteggio
                }

            }else{

                this.currentStreak = 0;

                if(this.currentMistakes >= 4){
                    this.puzzlesLost++;
                    this.mistakeHistogram[4]++;     //Persi
                }else{
                    this.mistakeHistogram[5]++;     //Non finiti
                }
            }
        }

        
        //Ora preparo per la nuova partita
        this.currentGameId = gameId;

        if(this.currentFoundGroups == null){    //Sempre perché Gson quando carica un oggetto può ignorare il costruttore.
            this.currentFoundGroups = new HashSet<>();

        }else{
            this.currentFoundGroups.clear();
        }

        this.currentMistakes = 0;
        this.currentScore = 0;
        this.gameFinished = false;
    }
    

    public Integer getCurrentGameId(){
        return currentGameId;
    }

    public Set<String> getCurrentFoundGroups(){
        return currentFoundGroups;
    }
    
    public int getCurrentMistakes(){
        return currentMistakes;
    }
    
    public int getCurrentScore(){
        return currentScore;
    }
    
    public boolean isGameFinished(){
        return gameFinished;
    }
    

    public void addMistake(){
        this.currentMistakes++; 
        this.currentScore -= 4;

        if(this.currentMistakes >= 4){
            this.gameFinished = true;
        }
    }


    public void addFoundGroup(String theme){ 
        this.currentFoundGroups.add(theme);

        //Risposta corretta
        this.currentScore += 6;
        
        //Quarto gruppo è per esclusione
        if (this.currentFoundGroups.size() >= 3){
            this.gameFinished = true;
        }
    }

    public void setGameFinished(boolean finished){
        this.gameFinished = finished;
    }

    

    //getters per le statistiche e storico
    public Map<Integer, GameResult> getHistory(){
        return history;
    }
    
    public int getPuzzlesCompleted(){
        return puzzlesCompleted;
    }
    
    public int getPuzzlesWon(){
        return puzzlesWon;
    }

    public int getPuzzlesLost(){
        return puzzlesLost;
    }

    public int getCurrentStreak(){
        return currentStreak;
    }
    
    public int getMaxStreak(){
        return maxStreak;
    }
    
    public int getPerfectPuzzles(){
        return perfectPuzzles;
    }
    
    public int[] getMistakeHistogram(){
        return mistakeHistogram;
    }

}
