package common;

import java.util.List;

//MAPPO richiesta Json da Client al Server
//Se un campo non è usato ina una operazione è null


public class JsonRequest{
    public String operation;    //Da questo deciso cosa fare in process di CommmandProcessor
                                //Costanti definite in "Costants.java"
    
    //Campi dell'utente
    public String name;         //Questo lo uso per registrare
    public String psw;
    public String username;     //Questo lo uso per login
    
    //Per submitProposal
    public List<String> words;
    
    //Per updateCredentials
    public String oldName;
    public String newName;
    public String oldPsw;
    public String newPsw;
    
    //Per info e stats
    public Integer gameId;
    
    //Per leaderboard
    public String playerName;
    public Integer topPlayers;
    
    //Porta UDP mandata a login
    public Integer udpPort;
    
    public JsonRequest(String operation){
        this.operation = operation;
    }
}
