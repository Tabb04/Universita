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

/**
 * Elabora i comandi inviati dal client in base al campo 'operation'.
 * Ogni metodo implementa le regole e produce un JsonResponse coerente.
 */
public class CommandProcessor {

    public static JsonResponse process(JsonRequest req, SocketChannel channel, NioServer server) {
        if (req == null || req.operation == null) {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Operazione non specificata.");
        }

        switch (req.operation) {
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

    private static JsonResponse handleRegister(JsonRequest req, NioServer server) {
        if (req.name == null || req.psw == null || req.name.trim().isEmpty()) {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Nome utente o password mancanti.");
        }
        
        String username = req.name.trim();
        synchronized (server.getRegisteredUsers()) {
            if (server.getRegisteredUsers().containsKey(username)) {
                return JsonResponse.error(Constants.ERR_ALREADY_REGISTERED, "L'utente " + username + " esiste già.");
            }
            User newUser = new User(username, req.psw);
            server.getRegisteredUsers().put(username, newUser);
        }
        return JsonResponse.success(new JsonObject());
    }

    private static JsonResponse handleLogin(JsonRequest req, SocketChannel channel, NioServer server) {
        if (req.username == null || req.psw == null) {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Username o password mancanti.");
        }
        
        User user = server.getRegisteredUsers().get(req.username);
        if (user == null || !user.getPassword().equals(req.psw)) {
            return JsonResponse.error(Constants.ERR_AUTH_FAILED, "Credenziali errate.");
        }
        
        server.getActiveConnections().put(channel, user);
        
        if (req.udpPort != null) {
            try {
                InetSocketAddress remoteAddress = (InetSocketAddress) channel.getRemoteAddress();
                InetSocketAddress udpAddress = new InetSocketAddress(remoteAddress.getAddress(), req.udpPort);
                server.getUserUdpAddresses().put(user.getUsername(), udpAddress);
            } catch (Exception e) {
                System.err.println("Errore IP client per UDP: " + e.getMessage());
            }
        }

        Game game = server.getGameManager().getCurrentGame();
        JsonObject data = new JsonObject();
        data.addProperty("message", "Login completato. Benvenuto " + user.getUsername());
        
        if (game != null) {
            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());
            Gson gson = new Gson();
            JsonArray wordsArray = gson.toJsonTree(game.getAllWords()).getAsJsonArray();
            data.add("words", wordsArray);
            
            if (user.getCurrentGameId() == null || game.getGameId() != user.getCurrentGameId()) {
                user.resetGameState(game.getGameId());
            }
            
            // Aggiungiamo campi extra per login (come da specifica: proposte corrette, num errori, punti)
            data.add("correctProposals", gson.toJsonTree(user.getCurrentFoundGroups()));
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
        }
        return JsonResponse.success(data);
    }
    
    private static JsonResponse handleLogout(SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().remove(channel);
        if (user != null) {
            server.getUserUdpAddresses().remove(user.getUsername());
            return JsonResponse.success(new JsonObject());
        }
        return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Utente non loggato.");
    }
    
    private static JsonResponse handleSubmitProposal(JsonRequest req, SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Utente non loggato.");
        
        if (req.words == null || req.words.size() != 4) {
            return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Devi inviare esattamente 4 parole.");
        }
        Game game = server.getGameManager().getCurrentGame();
        if (game == null) {
            return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Nessuna partita globale in corso.");
        }
        if (user.getCurrentGameId() == null || game.getGameId() != user.getCurrentGameId()) {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Tempo scaduto per questa partita! Aspetta la prossima.");
        }
        
        synchronized (user) {
            if (user.isGameFinished()) {
                return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Hai già terminato questa partita.");
            }

            // Validazione malformata (parole duplicate, non nel gioco, già assegnate)
            Set<String> uniqueWords = new HashSet<>(req.words);
            if (uniqueWords.size() != 4) {
                return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: parole duplicate o numero errato.");
            }
            List<String> allWords = game.getAllWords();
            for (String w : req.words) {
                if (!allWords.contains(w)) {
                    return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: la parola " + w + " non appartiene alla partita corrente.");
                }
            }
            for (String theme : user.getCurrentFoundGroups()) {
                for (WordDataset.Group g : game.getData().groups) {
                    if (g.theme.equals(theme)) {
                        for (String w : req.words) {
                            if (g.words.contains(w)) {
                                return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Proposta malformata: la parola " + w + " è già stata raggruppata correttamente.");
                            }
                        }
                    }
                }
            }

            WordDataset.Group foundGroup = game.checkProposal(req.words);
            JsonObject data = new JsonObject();
            
            if (foundGroup != null) {
                if (user.getCurrentFoundGroups().contains(foundGroup.theme)) {
                    return JsonResponse.error(Constants.ERR_INVALID_PROPOSAL, "Hai già indovinato questo gruppo.");
                }
                user.addFoundGroup(foundGroup.theme);
                data.addProperty("result", "correct");
                data.addProperty("theme", foundGroup.theme);
            } else {
                user.addMistake();
                data.addProperty("result", "wrong");
            }
            
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
            data.addProperty("gameFinished", user.isGameFinished());
            
            return JsonResponse.success(data);
        }
    }

    private static JsonResponse handleUpdateCredentials(JsonRequest req, SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) {
            return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Devi essere loggato.");
        }
        if (req.oldName == null || req.oldPsw == null) {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Devi inserire le tue vecchie credenziali per aggiornarle.");
        }
        if (!user.getPassword().equals(req.oldPsw) || !user.getUsername().equals(req.oldName)) {
            return JsonResponse.error(Constants.ERR_AUTH_FAILED, "Vecchie credenziali errate.");
        }
        
        boolean changed = false;
        
        // Cambio password
        if (req.newPsw != null && !req.newPsw.isEmpty() && !req.newPsw.equals("-")) {
            user.setPassword(req.newPsw);
            changed = true;
        }
        
        // Cambio nome utente (più complesso perché è la chiave della mappa registeredUsers)
        if (req.newName != null && !req.newName.isEmpty() && !req.newName.equals("-")) {
            synchronized (server.getRegisteredUsers()) {
                if (server.getRegisteredUsers().containsKey(req.newName)) {
                    return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Il nuovo nome utente esiste già.");
                }
                // Rimozione vecchia chiave e inserimento nuova
                server.getRegisteredUsers().remove(user.getUsername());
                user.setUsername(req.newName);
                server.getRegisteredUsers().put(req.newName, user);
                changed = true;
            }
        }
        
        if (changed) {
            JsonObject data = new JsonObject();
            data.addProperty("message", "Credenziali aggiornate con successo.");
            return JsonResponse.success(data);
        } else {
            return JsonResponse.error(Constants.ERR_INVALID_REQUEST, "Nessun parametro da aggiornare.");
        }
    }

    private static JsonResponse handleGameInfo(JsonRequest req, SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        
        Game game = server.getGameManager().getCurrentGame();
        Gson gson = new Gson();
        JsonObject data = new JsonObject();
        
        boolean isCurrentGame = (req.gameId == null || req.gameId == -1 || (game != null && game.getGameId() == req.gameId));
        
        if (isCurrentGame && game != null) {
            // Partita in corso
            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());
            data.add("words", gson.toJsonTree(game.getAllWords()).getAsJsonArray());
            data.add("correctProposals", gson.toJsonTree(user.getCurrentFoundGroups()));
            data.addProperty("mistakes", user.getCurrentMistakes());
            data.addProperty("score", user.getCurrentScore());
            return JsonResponse.success(data);
        } else {
            // Partita conclusa
            int queryGameId = (req.gameId != null && req.gameId != -1) ? req.gameId : (game != null ? game.getGameId() : -1);
            GameManager.HistoricalGameStats stats = server.getGameManager().getHistoricalStats(queryGameId);
            if (stats == null) {
                return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Partita " + queryGameId + " inesistente o non ancora conclusa.");
            }
            data.addProperty("gameId", stats.gameId);
            data.add("groups", gson.toJsonTree(stats.data.groups));
            
            // Dati utente specifici per quella partita
            User.GameResult result = user.getHistory().get(queryGameId);
            if (result != null) {
                data.addProperty("myMistakes", result.mistakes);
                data.addProperty("myScore", result.score);
                data.addProperty("won", result.won);
            } else {
                data.addProperty("participated", false);
            }
            return JsonResponse.success(data);
        }
    }
    
    private static JsonResponse handleGameStats(JsonRequest req, SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        
        Game game = server.getGameManager().getCurrentGame();
        JsonObject data = new JsonObject();
        boolean isCurrentGame = (req.gameId == null || req.gameId == -1 || (game != null && game.getGameId() == req.gameId));
        
        if (isCurrentGame && game != null) {
            // Partita in corso: tempo rimanente, numero di giocatori totali, quanti hanno concluso e quanti vinto.
            int playing = 0, finished = 0, won = 0;
            for (User u : server.getRegisteredUsers().values()) {
                if (u.getCurrentGameId() != null && u.getCurrentGameId() == game.getGameId()) {
                    if (!u.isGameFinished()) playing++;
                    if (u.isGameFinished()) finished++;
                    if (u.getCurrentFoundGroups() != null && u.getCurrentFoundGroups().size() >= 3) won++;
                }
            }
            data.addProperty("gameId", game.getGameId());
            data.addProperty("remainingTimeSeconds", game.getRemainingTimeSeconds());
            data.addProperty("playersPlaying", playing);
            data.addProperty("playersFinished", finished);
            data.addProperty("playersWon", won);
            return JsonResponse.success(data);
        } else {
            // Partita conclusa
            int queryGameId = (req.gameId != null && req.gameId != -1) ? req.gameId : (game != null ? game.getGameId() : -1);
            GameManager.HistoricalGameStats stats = server.getGameManager().getHistoricalStats(queryGameId);
            if (stats == null) {
                return JsonResponse.error(Constants.ERR_GAME_NOT_FOUND, "Statistiche della partita " + queryGameId + " non disponibili.");
            }
            data.addProperty("gameId", stats.gameId);
            data.addProperty("totalParticipants", stats.totalParticipants);
            data.addProperty("finishedParticipants", stats.finishedParticipants);
            data.addProperty("wonParticipants", stats.wonParticipants);
            data.addProperty("averageScore", stats.averageScore);
            return JsonResponse.success(data);
        }
    }
    
    private static JsonResponse handlePlayerStats(SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        
        JsonObject data = new JsonObject();
        data.addProperty("username", user.getUsername());
        data.addProperty("puzzlesCompleted", user.getPuzzlesCompleted());
        double winRate = user.getPuzzlesCompleted() > 0 ? (double) user.getPuzzlesWon() / user.getPuzzlesCompleted() * 100 : 0;
        double lossRate = user.getPuzzlesCompleted() > 0 ? (double) user.getPuzzlesLost() / user.getPuzzlesCompleted() * 100 : 0;
        data.addProperty("winRate", String.format("%.2f%%", winRate));
        data.addProperty("lossRate", String.format("%.2f%%", lossRate));
        data.addProperty("currentStreak", user.getCurrentStreak());
        data.addProperty("maxStreak", user.getMaxStreak());
        data.addProperty("perfectPuzzles", user.getPerfectPuzzles());
        
        Gson gson = new Gson();
        data.add("mistakeHistogram", gson.toJsonTree(user.getMistakeHistogram()));
        
        return JsonResponse.success(data);
    }
    
    private static JsonResponse handleLeaderboard(JsonRequest req, SocketChannel channel, NioServer server) {
        User user = server.getActiveConnections().get(channel);
        if (user == null) return JsonResponse.error(Constants.ERR_NOT_LOGGED_IN, "Non loggato.");
        
        // Ordina utenti in base al punteggio globale storico
        List<User> userList = new ArrayList<>(server.getRegisteredUsers().values());
        userList.sort((u1, u2) -> Integer.compare(u2.getGlobalScore(), u1.getGlobalScore())); 
        
        JsonObject data = new JsonObject();
        
        if (req.playerName != null && !req.playerName.isEmpty()) {
            // Ricerca posizione giocatore specifico
            int rank = -1;
            int score = 0;
            for (int i = 0; i < userList.size(); i++) {
                if (userList.get(i).getUsername().equals(req.playerName)) {
                    rank = i + 1;
                    score = userList.get(i).getGlobalScore();
                    break;
                }
            }
            if (rank != -1) {
                data.addProperty("playerName", req.playerName);
                data.addProperty("rank", rank);
                data.addProperty("score", score);
                return JsonResponse.success(data);
            } else {
                return JsonResponse.error(Constants.ERR_USER_NOT_FOUND, "Giocatore non trovato.");
            }
        } else {
            // Ritorna i top K
            int limit = (req.topPlayers != null && req.topPlayers > 0) ? req.topPlayers : userList.size(); 
            JsonArray leaderArray = new JsonArray();
            for (int i = 0; i < Math.min(limit, userList.size()); i++) {
                User u = userList.get(i);
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
