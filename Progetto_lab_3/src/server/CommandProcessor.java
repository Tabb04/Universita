package server;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.net.InetSocketAddress;
import java.nio.channels.SocketChannel;
import common.JsonRequest;
import common.JsonResponse;
import common.Constants;
import server.User;
import server.Game;
import server.WordDataset;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.HashSet;


//ELABORA I COMANDI INVIATI DAL CLIENT SMISTANDO IN BASE AL CAMPO OPERATION
//VIENE PRODOTTA UNA JSONRESPONSE

public class CommandProcessor{

    public static JsonResponse process(JsonRequest req, SocketChannel channel, NioServer server){

        //Controlli di sicurezza visto che li faccio anche nel client.
        if(req == null || req.operation == null){
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Operazione non specificata.");
        }

        switch(req.operation){

            case Constants.OP_REGISTER:
                return handleRegister(req, server);

            case Constants.OP_LOGIN:
                return handleLogin(req, channel, server);

            case Constants.OP_LOGOUT:
                return handleLogout(channel, server);

            case Constants.OP_SUBMIT_PROPOSAL:
                return handleSubmitProposal(req, channel, server);

            case Constants.OP_UPDATE_CREDENTIALS:
                return handleUpdateCredentials(req, channel, server);

            case Constants.OP_REQUEST_GAME_INFO:
                return handleGameInfo(req, channel, server);

            case Constants.OP_REQUEST_GAME_STATS:
                return handleGameStats(req, channel, server);

            case Constants.OP_REQUEST_LEADERBOARD:
                return handleLeaderboard(req, channel, server);

            case Constants.OP_REQUEST_PLAYER_STATS:
                return handlePlayerStats(channel, server);

            default:
                return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Operazione sconosciuta: " + req.operation);
        }
    }



    //TUTTI GLI HANDLE:
//---------------------------------------------------------------------------------------


    //REGISTRAZIONE
    private static JsonResponse handleRegister(JsonRequest req, NioServer server){

        if(req.name == null || req.psw == null || req.name.trim().isEmpty()){
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Nome utente o password mancanti.");
        }

        
        String name = req.name.trim();

        //Prendo la mappa degli utenti registrati
        //Faccio questa parte esclusiva per evitare doppie registrazioni
        synchronized(server.getRegisteredUsers()){
            if(server.getRegisteredUsers().containsKey(name)){
                return JsonResponse.error(Constants.ERR_ALREADY_REGISTERED, "Utente " + name + " esiste già.");
            }

            User newUser = new User(name, req.psw);
            server.getRegisteredUsers().put(name, newUser);
        }
        return JsonResponse.success(new JsonObject());  //Payload vuoto
    }


//---------------------------------------------------------------------------------------

    //LOGIN
    private static JsonResponse handleLogin(JsonRequest req, SocketChannel channel, NioServer server){

        if(req.username == null || req.psw == null){
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Username o password mancanti.");
        }
        
        User user = server.getRegisteredUsers().get(req.username);

        if(user == null || !user.getPassword().equals(req.psw)){
            return JsonResponse.error(Constants.ERR_AUTH_FAILED, "Credenziali errate.");
        }
        
        //Se supero tutto lo metto nella mappa delle socketchannel-user
        server.getActiveConnections().put(channel, user);
        System.out.println("[SERVER] Utente loggato: " + user.getUsername());
        
        //Per notifiche asincrone
        if(req.udpPort != null){    //La mando solo a login
            try{

                //Estraggo un ooggetto InetSocketAddress per ricavare l'IP per i messaggi UDP
                InetSocketAddress remoteAddress = (InetSocketAddress) channel.getRemoteAddress();

                //Punta all'IP dell'utente e alla porta udp data
                InetSocketAddress udpAddress = new InetSocketAddress(remoteAddress.getAddress(), req.udpPort);

                //Poi lo aggiungo alla mappa
                server.getUserUdpAddresses().put(user.getUsername(), udpAddress);

            }catch(Exception e){
                System.err.println("Errore IP client per UDP: " + e.getMessage());
            }
        }


        Game game = server.getGameManager().getCurrentGame();
        JsonObject data = new JsonObject();

        //Data sarà il payload
        data.addProperty("message", "Login completato. Benvenuto " + user.getUsername());
        
        if(game != null){   //Controllo per sicurezza

            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());

            Gson gson = new Gson();

            //Prendo l'elenco di parole, faccio un oggetto JsonArray e uso .getAsJsonArray per fare casting
            JsonArray wordsArray = gson.toJsonTree(game.getAllWords()).getAsJsonArray();
            data.add("words", wordsArray);
            
            //Controllo che abbia un GameId e che sia della partita corrente, nel caso lo aggiorno
            if(user.getCurrentGameId() == null || game.getGameId() != user.getCurrentGameId()){
                user.resetGameState(game.getGameId());
            }
            
            //Altri dati
            data.add("correctProposals", gson.toJsonTree(user.getCurrentFoundGroups()));
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
        }

        return JsonResponse.success(data);
    }


//---------------------------------------------------------------------------------------

    //LOGOUT
    private static JsonResponse handleLogout(SocketChannel channel, NioServer server){

        //Devo rimuoverlo sia dalle connessioni TCP che UDP
        User user = server.getActiveConnections().remove(channel);
        if(user != null){
            server.getUserUdpAddresses().remove(user.getUsername());
            System.out.println("[SERVER] Logout utente: " + user.getUsername());
            return JsonResponse.success(new JsonObject());  //Sempre payload vuoto
        }
        return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Utente non loggato.");
    }


//---------------------------------------------------------------------------------------

    //CONTROLLO TENTATIVI
    private static JsonResponse handleSubmitProposal(JsonRequest req, SocketChannel channel, NioServer server){

        //prendo lo user
        User user = server.getActiveConnections().get(channel);
        if(user == null){
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Utente non loggato!");
        }
        
        if(req.words == null || req.words.size() != 4){    //Ricontrollo che siano 4 parole comunque
            return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Devi inviare esattamente 4 parole!");
        }

        //Prendo la partita corrente 
        Game game = server.getGameManager().getCurrentGame();
        
        if(game == null){
            return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Nessuna partita globale in corso.");
        }



        if(user.getCurrentGameId() == null || game.getGameId() != user.getCurrentGameId()){

            //Prima non trasferivo utenti correttamente, lascio questo controllo comunque
            user.resetGameState(game.getGameId());
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Mismatch tra le partite, reset Id partita effettuato.");
        }
        
        
        if(user.isGameFinished()){  //Ho già fatto troppi errori o già vinto
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Hai già terminato questa partita.");
        }


        //Parole duplicate, non nel gioco o già assegnate
        Set<String> uniqueWords = new HashSet<>(req.words); //Creo un hashset per filtrare i duplicati

        if(uniqueWords.size() != 4){
            return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: parole duplicate o numero errato");
        }


        List<String> allWords = game.getAllWords();
        for(String wd : req.words){

            //Ciclando nelle parole inviate controllo se ce ne è almeno una che non appartiene alle parole della partita
            if(!allWords.contains(wd)){
                return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: la parola " + wd + " non appartiene alla partita corente.");
            }
        }


        //Controllo che le parole inviate non appartengano a gruppi già indovinati
        //Guardo tra i gruppi già trovati
        for(String theme : user.getCurrentFoundGroups()){

            //Per ogni tema guardo l'oggetto Group caricato dal database per sapere le parole del gruppo
            for(WordDataset.Group g : game.getData().groups){

                if(g.theme.equals(theme)){  //Se corrisponde

                    //Guardo le 4 parole della proposta
                    for(String w : req.words){

                        //Se faceva parte del gruppo allora errore
                        if(g.words.contains(w)){
                            return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: la parola " + w + " è già stata raggruppata");
                        }
                    }
                }
            }
        }

        //Ora guardo se la proposta è corretta e ricavo il gruppo
        WordDataset.Group foundGroup = game.checkProposal(req.words);
        JsonObject data = new JsonObject();      //Payload
        

        //Se è corretta
        if(foundGroup != null){

            //Controllato già sopra che non sia un gruppo già indovinato

            user.addFoundGroup(foundGroup.theme);
            data.addProperty("result", "correct");
            data.addProperty("theme", foundGroup.theme);
        }else{

            //Se risposta incorretta
            user.addMistake();
            data.addProperty("result", "wrong");
        }

            //gli invio i dati generali della partita in corso
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
            data.addProperty("gameFinished", user.isGameFinished());
            
            return JsonResponse.success(data);

    }


//---------------------------------------------------------------------------------------

    //Aggiornare credenziali
    //Se non voglio aggiornare uno dei due campi basta mettere un dash (-)
    private static JsonResponse handleUpdateCredentials(JsonRequest req, SocketChannel channel, NioServer server){


        //Nel pdf dice che l'utente deve essere in grado di dimostrare che conosce la password
        //Quindi faccio che questa operazione è posssibile anche da sloggati
        if(req.oldName == null || req.oldPsw == null){
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Inseririsci le tue vecchie credenziali per aggiornarle.");
        }
        
        //Vado a controllare se corrisponde la password
        User user = server.getRegisteredUsers().get(req.oldName);
        if(user == null || !user.getPassword().equals(req.oldPsw)){
            return JsonResponse.error(Constants.ERR_AUTH_FAILED, "Vecchie credenziali errate o utente inesistente.");
        }
        
        boolean changed = false;
        

        //Cambio password
        //Controllo che sia arrivata una password non vuota e non un dash
        if(req.newPsw != null && !req.newPsw.isEmpty() && !req.newPsw.equals("-")){ //usare equals non ""==""
            user.setPassword(req.newPsw);
            changed = true;
        }
        
        //Cambio nome utente (più complesso perché è la chiave della mappa registeredUsers)
        if(req.newName != null && !req.newName.isEmpty() && !req.newName.equals("-")){

            //Il cambio di nome lo faccio in un blocco syncrhonized per evitare che qualcun'altro si registri con lo stesso
            //nome nello stesso istante
            synchronized(server.getRegisteredUsers()){

                //Controllo che il nome utente non sia già preso da qualcuno
                if(server.getRegisteredUsers().containsKey(req.newName)){
                    return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Il nuovo nome utente esiste già.");
                }
                
                //Rimuovo vecchia chiave e inserisco la nuova
                server.getRegisteredUsers().remove(user.getUsername());
                user.setUsername(req.newName);
                server.getRegisteredUsers().put(req.newName, user);
                changed = true;
            }
        }
        

        if(changed){
            JsonObject data = new JsonObject();
            data.addProperty("message", "Credenziali aggiornate con successo.");
            return JsonResponse.success(data);
        }else{
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Nessun parametro da aggiornare.");
        }
    }


//---------------------------------------------------------------------------------------


    //DATI DI GIOCO E PROGRESSO INDIVIDUALE
    private static JsonResponse handleGameInfo(JsonRequest req, SocketChannel channel, NioServer server){
        
        User user = server.getActiveConnections().get(channel);
        if(user == null){
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        }
        
        Game game = server.getGameManager().getCurrentGame();
        Gson gson = new Gson();
        JsonObject data = new JsonObject(); //Sempre payload
        
        //Siccome posso richiedere anche informazioni su partite passate devo capire se
        //sta chiedendo uno storico o quella corrente (se corrente non invia niente o magari un -1 o l'id corrente se è infame)
        int targetId= (req.gameId == null || req.gameId == -1) ? game.getGameId() : req.gameId;


        //Partita in corso
        if(targetId == game.getGameId()){

            //Sempre per problema di mismatch faccio un controllo sui gameId
            if (user.getCurrentGameId() == null || game.getGameId() != user.getCurrentGameId()) {
                user.resetGameState(game.getGameId());
            }

            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());
            data.add("words", gson.toJsonTree(game.getAllWords()).getAsJsonArray());
            data.add("correctProposals", gson.toJsonTree(user.getCurrentFoundGroups()));
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
            return JsonResponse.success(data);


        //Partita conclusa
        }else{
            //Oggetto con le statistiche di una partita passata
            GameManager.HistoricalGameStats stats = server.getGameManager().getHistoricalStats(targetId);

            if(stats == null){
                return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Partita " + targetId + " inesistente o non ancora finita");
            }

            //Dati generali della partita
            data.addProperty("gameId", stats.gameId);
            data.add("groups", gson.toJsonTree(stats.data.groups));


            //Dati dell'utente specifico su quella partita
            User.GameResult result = user.getHistory().get(targetId);   //Prendo lo storico dell'utente e poi la partita specifica
            
            if(result != null){
                data.addProperty("myMistakes", result.mistakes);
                data.addProperty("myScore", result.score);
                data.addProperty("won", result.won);
            }else{
                data.addProperty("participated", false);
            }
            return JsonResponse.success(data);
        }
    }
    

//---------------------------------------------------------------------------------------


    //STATISTICHE GLOBALI SUI GIOCATORI DI QUELLA PARTITA
    private static JsonResponse handleGameStats(JsonRequest req, SocketChannel channel, NioServer server){

        User user = server.getActiveConnections().get(channel);
        if(user == null){
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        }

        Game game = server.getGameManager().getCurrentGame();
        JsonObject data = new JsonObject();

        //Stesso calcolo come in handleGameInfo
        int targetId = (req.gameId == null || req.gameId == -1) ? game.getGameId() : req.gameId;

        //Partita corrente
        if(targetId == game.getGameId()){

            //Voglio sapere tempo rimanente, numero di giocatori totali, quanti hanno concluso e quanti vinto
            int playing = 0;
            int finished = 0;
            int won = 0;

            //Ciclo su tutti gli utenti REGISTRATI
            //Potrei ciclare sulle connessioni attive ma un utente potrebbe aver giocato ed essersi
            //disconnesso prima della richiesta di statistiche
            for(User u : server.getRegisteredUsers().values()){

                //Guardo se il GameId corrisponde e lo sommo alle statistiche
                if(u.getCurrentGameId() != null && u.getCurrentGameId() == game.getGameId()){
                    if(!u.isGameFinished()){
                        playing++;
                    }

                    if(u.isGameFinished()){
                        finished++;
                    }

                    if (u.getCurrentFoundGroups() != null && u.getCurrentFoundGroups().size() >= 3){
                        won++;
                    }
                }
            }

            //Statistiche sul gioco
            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());
            data.addProperty("playersPlaying", playing);
            data.addProperty("playersFinished", finished);
            data.addProperty("playersWon", won);
            return JsonResponse.success(data);

        // Partita conclusa    
        }else{

            //Uguale a info
            GameManager.HistoricalGameStats stats = server.getGameManager().getHistoricalStats(targetId);
            
            if(stats == null){
                return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Statistiche della partita " + targetId + " non disponibili.");
            }

            //Dati
            data.addProperty("gameId", stats.gameId);
            data.addProperty("totalParticipants", stats.totalParticipants);
            data.addProperty("finishedParticipants", stats.finishedParticipants);
            data.addProperty("wonParticipants", stats.wonParticipants);
            data.addProperty("averageScore", stats.averageScore);
            return JsonResponse.success(data);
        }
    }


//---------------------------------------------------------------------------------------

    //FORNISCE STATISTICHE SULL'UTENTE CHE FA RICHIESTA
    private static JsonResponse handlePlayerStats(SocketChannel channel, NioServer server){

        User user = server.getActiveConnections().get(channel);
        if(user == null){
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        }

        JsonObject data = new JsonObject();
        data.addProperty("username", user.getUsername());
        data.addProperty("puzzlesCompleted", user.getPuzzlesCompleted());

        //Per il win e lossrate devo avere almeno una partita completata
        double winRate = user.getPuzzlesCompleted() > 0 ? (double) user.getPuzzlesWon() / user.getPuzzlesCompleted() * 100 : 0;
        double lossRate = user.getPuzzlesCompleted() > 0 ? (double) user.getPuzzlesLost() / user.getPuzzlesCompleted() * 100 : 0;
        
        data.addProperty("winRate", String.format("%.2f%%", winRate));
        data.addProperty("lossRate", String.format("%.2f%%", lossRate));
        data.addProperty("currentStreak", user.getCurrentStreak());
        data.addProperty("maxStreak", user.getMaxStreak());
        data.addProperty("perfectPuzzles", user.getPerfectPuzzles());
        
        Gson gson = new Gson();

        //Vettore con le statistiche
        data.add("mistakeHistogram", gson.toJsonTree(user.getMistakeHistogram()));
        
        return JsonResponse.success(data);
    }


//---------------------------------------------------------------------------------------

    //LEADERBOARD DI TUTTI GLI UTENTI (non nel dettaglio come playerstats)
    //POSIZIONE DI UN UTENTE SPECIFICO NELLA LEADERBOARD
    //TOP K UTENTI NELLA LEADERBOARD
    private static JsonResponse handleLeaderboard(JsonRequest req, SocketChannel channel, NioServer server){

        User user = server.getActiveConnections().get(channel);
        if (user == null){
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        }


        //Ordino utenti in base al punteggio storico:
        //Creo una lista con tutti gli user
        List<User> userList = new ArrayList<>(server.getRegisteredUsers().values());
        //Ordino per globalscore
        userList.sort((u1, u2) -> Integer.compare(u2.getGlobalScore(), u1.getGlobalScore()));         


        JsonObject data = new JsonObject();

        //Ha richiesto un utente specifico
        if(req.playerName != null && !req.playerName.isEmpty()){

            int rank = -1;
            int score = 0;

            //Scorro nella lista degli utenti
            for(int i = 0; i < userList.size(); i++){

                //Se lo trovo stampo il rank e globalscore
                if(userList.get(i).getUsername().equals(req.playerName)){
                    rank = i + 1;
                    score = userList.get(i).getGlobalScore();
                    break;
                }
            }

            //Se è stato trovato
            if(rank != -1){

                data.addProperty("playerName", req.playerName);
                data.addProperty("rank", rank);
                data.addProperty("score", score);
                return JsonResponse.success(data);

            }else{
                return JsonResponse.error(Constants.ERR_USER_NOT_FOUND, "Giocatore non trovato.");
            }

        //Non ha specificato un utente quindi stampo o tutta leaderboard o top k
        }else{

            //Guardo se il campo di quanti k utenti voglio vedere è settato
            int k = (req.topPlayers != null && req.topPlayers > 0) ? req.topPlayers : userList.size();

            JsonArray leaderArray = new JsonArray();

            //Fino alla fine (forza juve) o fino a k
            for(int i = 0; i < Math.min(k, userList.size()); i++){

                User u = userList.get(i);

                //Oggetto che metto nel payload
                JsonObject entry = new JsonObject();
                entry.addProperty("rank", i + 1);
                entry.addProperty("username", u.getUsername());
                entry.addProperty("globalScore", u.getGlobalScore());
                leaderArray.add(entry);
            }

            data.add("topPlayers", leaderArray);
            return JsonResponse.success(data);
        }
    }


}
