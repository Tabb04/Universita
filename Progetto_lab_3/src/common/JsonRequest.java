package common;

import java.util.List;

/**
 * Mappa una richiesta JSON dal Client al Server.
 * Contiene tutti i possibili campi per semplicità (saranno null se non usati in una specifica operation).
 */
public class JsonRequest {
    public String operation;
    
    // Campi utente
    public String name;
    public String psw;
    public String username;
    
    // Per submitProposal
    public List<String> words;
    
    // Per updateCredentials
    public String oldName;
    public String newName;
    public String oldPsw;
    public String newPsw;
    
    // Per info e stats
    public Integer gameId;
    
    // Per leaderboard
    public String playerName;
    public Integer topPlayers;
    
    // Campo aggiuntivo per scambiare la porta UDP
    public Integer udpPort;
    
    public JsonRequest(String operation) {
        this.operation = operation;
    }
}
