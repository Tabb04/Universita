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
    
    /*
    Ogni oggetto GameData è un singolo livello del gioco.
        -gameId è l'identificativo univoco che utilizzo sia per capire quale partita prendere sia per
        loggare al client.
        -groups è la lista di "4 parole e un tema".
     */

    public static class GameData{
        public int gameId;
        public List<Group> groups;
    }
    
    public static List<GameData> loadDataset(String filePath){
        
        //try with resources come a lezione
        try(FileReader reader = new FileReader(filePath)){
            Gson gson = new Gson();

            //Per evitare type erasure (a runtime diventerebbe tipo List generico)
            //Uso come spiegato a lezione TypeToken

            Type listType = new TypeToken<List<GameData>>(){}.getType();
            //Uso il tipo che ho estratto
            return gson.fromJson(reader, listType);
        
        }catch (Exception e) {
            System.err.println("Errore nel caricamento del dataset di parole: " + e.getMessage());
            return null;
        }
    }
}
