package server;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import java.io.FileReader;
import java.lang.reflect.Type;
import java.util.List;

public class WordDataset {
    public static class Group {
        public String theme;
        public List<String> words;
    }
    
    public static class GameData {
        public int gameId;
        public List<Group> groups;
    }
    
    public static List<GameData> loadDataset(String filePath) {
        try (FileReader reader = new FileReader(filePath)) {
            Gson gson = new Gson();
            Type listType = new TypeToken<List<GameData>>(){}.getType();
            return gson.fromJson(reader, listType);
        } catch (Exception e) {
            System.err.println("Errore nel caricamento del dataset di parole: " + e.getMessage());
            return null;
        }
    }
}
