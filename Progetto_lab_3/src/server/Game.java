package server;

import java.util.*;


//RAPPRESENTA LO STATO DI UNA PARTITA GLOBALMENTE

public class Game{
    private final int gameId;
    private final WordDataset.GameData data;
    private final long startTimeMillis;
    private final long durationMillis;
    private final List<String> allWords;    //Tutte le 16 parole
    

    public Game(WordDataset.GameData data, long durationSeconds){
        this.gameId = data.gameId;
        this.data = data;
        this.startTimeMillis = System.currentTimeMillis();
        this.durationMillis = durationSeconds * 1000;
        

        //Itero per aggiungere tutte le parole della partita
        List<String> words = new ArrayList<>();
        for(WordDataset.Group g : data.groups) {
            words.addAll(g.words);
        }

        Collections.shuffle(words, new Random());   //Non c'è bisogno usi il random efficiente per thread
        this.allWords = words;
    }
    

    public int getGameId(){
        return gameId;
    }
    
    public List<String> getAllWords(){
        return allWords;
    }
    
    public long getRemainingTimeSeconds(){
        return (durationMillis - (System.currentTimeMillis() - startTimeMillis)) / 1000;
    }

    
    public WordDataset.GameData getData(){
        return data;
    }
    
    public WordDataset.Group checkProposal(List<String> proposal)
    {
        if (proposal == null) return null;
        for (WordDataset.Group g : data.groups){

            //Uso un hashset perché non mi improta dell'ordine
            if (new HashSet<>(g.words).equals(new HashSet<>(proposal))) {
                return g;
            }
        }
        return null;
    }

}
