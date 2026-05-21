package client;

import common.ConfigReader;
import common.JsonRequest;
import common.Constants;
import client.NioClient;

import java.util.Arrays;
import java.util.Scanner;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


//MAIN DEL CLIENT. LEGGE DA RIGA DI COMANDO IN MODO BLOCCANTE E INVIA AL SERVER CON NIOCLIENT
public class ClientMain{
    public static void main(String[] args){
        System.out.println("Avvio Client di Connections...");
        
        //1. CARICO CONFIGURAZIONE
        ConfigReader config = new ConfigReader("config/client.properties");

        String address = config.getProperty("server.address", "127.0.0.1");

        int port = config.getIntProperty("server.port", 8080);
        
        //2. AVVIO NIOCLIENT (runnable)
        NioClient client = new NioClient(address, port);
        try{
            client.connect();
        }catch(Exception e){
            System.err.println("Impossibile connettersi al server: " + e.getMessage());
            return;
        }
        
        Thread networkThread = new Thread(client);
        networkThread.start();
        
        System.out.println("-------------------------------------");
        System.out.println("Comandi disponibili:");
        System.out.println("  register <nome> <pwd>");
        System.out.println("  login <nome> <pwd>");
        System.out.println("  logout");
        System.out.println("  updateCredentials <oldName> <oldPwd> <newName> <newPwd> (usa - per non cambiare)");
        System.out.println("  submitProposal <w1> <w2> <w3> <w4>");
        System.out.println("  requestGameInfo [gameId]");
        System.out.println("  requestGameStats [gameId]");
        System.out.println("  requestPlayerStats");
        System.out.println("  requestLeaderboard [playerName | topPlayers <N>]");
        System.out.println("  exit");
        System.out.println("-------------------------------------");
        
        //3. LEGGO COMANDI
        Scanner scanner = new Scanner(System.in);

        while(true){
            System.out.print("> ");
            
            //Se non arriva EOF
            if(!scanner.hasNextLine()){
                break;
            }
            

            String line = scanner.nextLine().trim();
            if(line.isEmpty()){
                continue;
            }
            
            List<String> list = new ArrayList<>();

            //Alcune parole nel gioco hanno uno spazio quindi devo identificarle con le virgolette ("")
            Matcher m = Pattern.compile("([^\"\\s]+|\"([^\"]*)\")\\s*").matcher(line);

            while(m.find()){
                if(m.group(2) != null){
                    list.add(m.group(2));   //Riconosce le stringhe racchiuse tra virgolette
                }else{
                    list.add(m.group(1));   //Riconosce le stringhe normali
                }
            }

            //Converto in un array di stringhe (dimensione della lista uguale a list.size())
            String[] parts = list.toArray(new String[0]);
            if (parts.length == 0){
                continue;
            }
            
            String cmd = parts[0];
            
            //In base all'operazione sarà la richiesta mandata al NioClient
            JsonRequest req = null;
            
            switch(cmd){

                case "register":
                    if(parts.length == 3){
                        req = new JsonRequest(Constants.OP_REGISTER);
                        req.name = parts[1];
                        req.psw = parts[2];

                    }else{
                        System.out.println("Errore. Uso: register <nome> <password>");
                    }
                    break;
                    
                case "login":
                    if(parts.length == 3){
                        req = new JsonRequest(Constants.OP_LOGIN);
                        req.username = parts[1];
                        req.psw = parts[2];
                        req.udpPort = client.getUdpPort();

                    }else{
                        System.out.println("Errore. Uso: login <nome> <password>");
                    }
                    break;

                case "logout":
                    //Richiesta di logout da mandare al server
                    req = new JsonRequest(Constants.OP_LOGOUT);
                    break;
                
                case "updateCredentials":
                    if(parts.length == 5){
                        req = new JsonRequest(Constants.OP_UPDATE_CREDENTIALS);
                        req.oldName = parts[1];
                        req.oldPsw = parts[2];
                        req.newName = parts[3];
                        req.newPsw = parts[4];

                    }else{
                        System.out.println("Errore. Uso: updateCredentials <oldNome> <oldPwd> <newName> <newPwd>");
                    }
                    break;

                case "submitProposal":
                    if(parts.length == 5){
                        req = new JsonRequest(Constants.OP_SUBMIT_PROPOSAL);
                        req.words = Arrays.asList(parts[1].toUpperCase(), parts[2].toUpperCase(), 
                                                  parts[3].toUpperCase(), parts[4].toUpperCase());
                    }else{
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
                case "exit":
                case "quit":
                case "close":
                case "shutdown":
                //Non me ne vengono altri in mente
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
