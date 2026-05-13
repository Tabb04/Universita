package server;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import java.io.FileReader;
import java.lang.reflect.Type;
import java.util.List;

public class WordDataset{
    public static class Group{
        public String theme;
        public List<String> words;
    }

    public static class GameData{
        public int gameId;
        public List<Group> groups;
    }


    //Voglio sapere il numero di games presenti per i bound del numero casuale
    public static int countGames(String filePath){
        int count = 0;

        //Devo usare questa formattazione altrimenti non mi funziona solo JsonReader
        try(com.google.gson.stream.JsonReader reader = new com.google.gson.stream.JsonReader(new FileReader(filePath))){
            //Uguale come a lezione
            reader.beginArray();
            while(reader.hasNext()){
                reader.skipValue();
                count++;
            }
            reader.endArray();
        }catch(Exception e){
            System.err.println("Errore durante il conteggio dei game: " + e.getMessage());
        }

        return count;
    }

    //Funzione per cercare un game dato l'indice casuale
    public static GameData loadGameAtIndex(String filePath, int targetIndex){
        try(com.google.gson.stream.JsonReader reader = new com.google.gson.stream.JsonReader(new FileReader(filePath))){
            Gson gson = new Gson();
            reader.beginArray();
            int currentIndex = 0;

            while(reader.hasNext()){
                if(currentIndex == targetIndex){
                    return gson.fromJson(reader, GameData.class);
                }else{
                    reader.skipValue();
                    currentIndex++;
                }
            }
            
            reader.endArray();
            
        }catch(Exception e){
            System.err.println("Errore nel caricamento del gioco con indice " + targetIndex + ": " + e.getMessage());
        }
        
        return null;
    }
}
