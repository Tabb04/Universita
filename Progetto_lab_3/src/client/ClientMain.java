package client;

import common.ConfigReader;
import common.JsonRequest;
import common.Constants;
import client.NioClient;

import java.util.Arrays;
import java.util.Scanner;

/**
 * Punto di ingresso per il client di Connections.
 * Legge comandi da riga di comando (CLI) in modo bloccante
 * e li invia al server tramite NioClient.
 */
public class ClientMain {
    public static void main(String[] args) {
        System.out.println("Avvio Client Connections...");
        
        ConfigReader config = new ConfigReader("config/client.properties");
        String address = config.getProperty("server.address", "127.0.0.1");
        int port = config.getIntProperty("server.port", 8080);
        
        NioClient client = new NioClient(address, port);
        try {
            client.connect();
        } catch (Exception e) {
            System.err.println("Impossibile connettersi al server: " + e.getMessage());
            return;
        }
        
        Thread networkThread = new Thread(client);
        networkThread.start();
        
        System.out.println("-------------------------------------");
        System.out.println("Comandi disponibili (Istruzioni.md):");
        System.out.println("  register <nome> <pwd>");
        System.out.println("  login <nome> <pwd>");
        System.out.println("  logout");
        System.out.println("  updateCredentials <oldName> <oldPwd> <newName> <newPwd> (usa - per non cambiare)");
        System.out.println("  submitProposal <w1> <w2> <w3> <w4>");
        System.out.println("  requestGameInfo [gameId]");
        System.out.println("  requestGameStats [gameId]");
        System.out.println("  requestPlayerStats");
        System.out.println("  requestLeaderboard [playerName | topPlayers <N>]");
        System.out.println("  quit");
        System.out.println("-------------------------------------");
        
        Scanner scanner = new Scanner(System.in);
        while (true) {
            System.out.print("> ");
            if (!scanner.hasNextLine()) break; 
            
            String line = scanner.nextLine().trim();
            if (line.isEmpty()) continue;
            
            String[] parts = line.split("\\s+");
            String cmd = parts[0];
            
            JsonRequest req = null;
            
            switch (cmd) {
                case "register":
                    if (parts.length == 3) {
                        req = new JsonRequest(Constants.OP_REGISTER);
                        req.name = parts[1];
                        req.psw = parts[2];
                    } else {
                        System.out.println("Errore. Uso: register <nome> <password>");
                    }
                    break;
                case "login":
                    if (parts.length == 3) {
                        req = new JsonRequest(Constants.OP_LOGIN);
                        req.username = parts[1];
                        req.psw = parts[2];
                        req.udpPort = client.getUdpPort();
                    } else {
                        System.out.println("Errore. Uso: login <nome> <password>");
                    }
                    break;
                case "logout":
                    req = new JsonRequest(Constants.OP_LOGOUT);
                    break;
                case "updateCredentials":
                    if (parts.length == 5) {
                        req = new JsonRequest(Constants.OP_UPDATE_CREDENTIALS);
                        req.oldName = parts[1];
                        req.oldPsw = parts[2];
                        req.newName = parts[3];
                        req.newPsw = parts[4];
                    } else {
                        System.out.println("Errore. Uso: updateCredentials <oldName> <oldPwd> <newName> <newPwd>");
                    }
                    break;
                case "submitProposal":
                    if (parts.length == 5) {
                        req = new JsonRequest(Constants.OP_SUBMIT_PROPOSAL);
                        req.words = Arrays.asList(parts[1].toUpperCase(), parts[2].toUpperCase(), 
                                                  parts[3].toUpperCase(), parts[4].toUpperCase());
                    } else {
                        System.out.println("Errore. Uso: submitProposal <w1> <w2> <w3> <w4>");
                    }
                    break;
                case "requestGameInfo":
                    req = new JsonRequest(Constants.OP_REQUEST_GAME_INFO);
                    if (parts.length > 1) {
                        try { req.gameId = Integer.parseInt(parts[1]); } catch (Exception e) {}
                    }
                    break;
                case "requestGameStats":
                    req = new JsonRequest(Constants.OP_REQUEST_GAME_STATS);
                    if (parts.length > 1) {
                        try { req.gameId = Integer.parseInt(parts[1]); } catch (Exception e) {}
                    }
                    break;
                case "requestPlayerStats":
                    req = new JsonRequest(Constants.OP_REQUEST_PLAYER_STATS);
                    break;
                case "requestLeaderboard":
                    req = new JsonRequest(Constants.OP_REQUEST_LEADERBOARD);
                    if (parts.length > 1) {
                        if (parts[1].equalsIgnoreCase("topPlayers") && parts.length > 2) {
                            try { req.topPlayers = Integer.parseInt(parts[2]); } catch(Exception e){}
                        } else {
                            req.playerName = parts[1];
                        }
                    }
                    break;
                case "quit":
                case "exit":
                    System.out.println("Chiusura client...");
                    System.exit(0);
                    break;
                case "help":
                    System.out.println("Comandi disponibili:");
                    System.out.println("register, login, logout, updateCredentials, submitProposal, requestGameInfo, requestGameStats, requestPlayerStats, requestLeaderboard, quit");
                    break;
                default:
                    System.out.println("Comando sconosciuto.");
                    break;
            }
            
            if (req != null) {
                client.sendRequest(req);
            }
        }
    }
}
