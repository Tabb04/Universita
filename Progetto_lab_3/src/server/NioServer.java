package server;

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

import java.util.*;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.charset.StandardCharsets;

import java.util.concurrent.*;

import common.JsonRequest;
import common.JsonResponse;
import server.GameManager;
import server.User;


//SERVER NIO
//Accetta e smista connessioni. Usa Threadpool per elaborazione


public class NioServer{
    private final int port;
    private final ExecutorService workerPool;   //Optato per cachedThreadPool
    private GameManager gameManager;
    private final Gson gson;
    private Selector selector;
    private ServerSocketChannel serverChannel;
    private DatagramSocket udpSocket;

    //Mappa globale per salvare gli utenti registrati, associato ad ognuno il nome utente
    private final ConcurrentHashMap<String, User> registeredUsers = new ConcurrentHashMap<>();


    //Mappa per associare un SocketChannel a un utente loggato
    private final ConcurrentHashMap<SocketChannel, User> activeConnections = new ConcurrentHashMap<>();


    // Mappa per associare un utente loggato alla sua porta UDP per notifiche asincrone
    private final ConcurrentHashMap<String, InetSocketAddress> userUdpAddresses = new ConcurrentHashMap<>();


    public NioServer(int port){
        this.port = port;
        this.workerPool = Executors.newCachedThreadPool();
        this.gson = new Gson();
    }

    public void setGameManager(GameManager gameManager){
        this.gameManager = gameManager;
    }

    public GameManager getGameManager(){
        return this.gameManager;
    }

    public ConcurrentHashMap<String, User> getRegisteredUsers(){
        return this.registeredUsers;
    }

    public ConcurrentHashMap<SocketChannel, User> getActiveConnections(){
        return this.activeConnections;
    }

    public ConcurrentHashMap<String, InetSocketAddress> getUserUdpAddresses(){
        return this.userUdpAddresses;
    }



    //AVVIO SERVER NIO E ASCOLTO SULLA PORTA DATA
    public void start() throws IOException{

        selector = Selector.open();


        //Nella soluzione dell'ultimo assigment ho visto che skippa .socket()
        //e fa la bind direttamente sull'oggetto ServerSocketChannel quindi faccio anche qui così
        serverChannel = ServerSocketChannel.open();

        serverChannel.bind(new InetSocketAddress(port));
        
        serverChannel.configureBlocking(false);
        
        serverChannel.register(selector, SelectionKey.OP_ACCEPT);


        //Apriamo un socket UDP per l'invio delle notifiche.
        udpSocket = new DatagramSocket();

        System.out.println("[SERVER] In ascolto sulla porta TCP " + port);



        //LOOP SELECTOR
        while(true){

            //Tutto uguale come a lezione
            selector.select();
            Set <SelectionKey> selectedKeys = selector.selectedKeys();
            Iterator <SelectionKey> iter = selectedKeys.iterator();

            while(iter.hasNext()){
                SelectionKey key = iter.next();
                iter.remove();

                //Non faccio la try, controllo su cosa potrebbe darmi l'eccezione
                if(!key.isValid())
                    continue;

                //Casi trattati sotto
                if(key.isAcceptable()){
                    acceptConnection(key);

                }else if(key.isReadable()){
                    handleRead(key);   
                }
            }
        }
    }


//---------------------------------------------------------------------------------------


    //ACCETTA CONNESSIONE
    private void acceptConnection(SelectionKey key) throws IOException{

        ServerSocketChannel server = (ServerSocketChannel) key.channel();
        SocketChannel client = server.accept();
        client.configureBlocking(false);

        client.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(8192)); //8KB ci entra tutto, predefinito dei buffered stream
        System.out.println("[SERVER] Nuova connessione TCP da " + client.getRemoteAddress());
    }


//---------------------------------------------------------------------------------------


    //LEGGO STRINGA JSON DAL CANALE E LA PASSO AL WORKER
    private void handleRead(SelectionKey key){
        SocketChannel clientChannel = (SocketChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();

        try{
            int bytesRead = clientChannel.read(buffer);

            if(bytesRead == -1){    //Ha mandato chiusura

                //Client ha chiuso la connessione
                handleDisconnection(clientChannel);
                return;
            }

            //Preparo per lettura
            buffer.flip();

            //Alloco array grande quanto ciò da leggere nel buffer
            byte[] data = new byte[buffer.remaining()];
            buffer.get(data);

            //Converto in una stringa che rappresenta la richiesta JSON
            String message = new String(data, StandardCharsets.UTF_8).trim();   //Trim per gli whitespace, nell'assigment usava solo un parametro
            buffer.compact();

            if(!message.isEmpty()){
                workerPool.submit(() -> processMessage(clientChannel, message));
            }
        }catch(IOException e){
            handleDisconnection(clientChannel);
        }
    }


//---------------------------------------------------------------------------------------


    //GESTISCO DISCONNESSIONE DELL'UTENTE
    private void handleDisconnection(SocketChannel clientChannel){
        try{
            //Per tcp il canale è la chiave, per udp è l'utente
            User u = activeConnections.remove(clientChannel);

            if(u != null){
                userUdpAddresses.remove(u.getUsername());
                System.out.println("[SERVER] Utente disconnesso: " + u.getUsername());
            }else{
                System.out.println("[SERVER] Client anonimo disconnesso");  //Se non era loggato
            }

            clientChannel.close();

        }catch(IOException e){
            e.printStackTrace();
        }
    }


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------


    //ESEGUITO DAL THREAD WORKER PER CALCOLARE RISPOSTA
    //Utilizza metodo statico di CommandProcessor
    private void processMessage(SocketChannel clientChannel, String message){
        try{
            
            //Oggetto JsonRequest con campi specifici per ogni "operation"
            JsonRequest request = gson.fromJson(message, JsonRequest.class);

            //Smistamento logica al processore
            JsonResponse response = CommandProcessor.process(request, clientChannel, this);

            //Invio risultato al client
            sendResponse(clientChannel, response);

        }catch(Exception e){
            e.printStackTrace();

            JsonResponse err = JsonResponse.error(400, "Errore server.");
            sendResponse(clientChannel, err);
        }
    }


//---------------------------------------------------------------------------------------


    //RISPOSTA SINCRONA IN JSON
    private void sendResponse(SocketChannel channel, JsonResponse response){
        try{

            //Aggiungo un \n al termine per aiutare il client a delimitare i messaggi
            String jsonStr = gson.toJson(response) + "\n";

            //Converto la stringa in byte
            byte[] responseBytes = jsonStr.getBytes(StandardCharsets.UTF_8);
            //Uso wrap per creare un bytebuffer per la risposta
            ByteBuffer buffer = ByteBuffer.wrap(responseBytes);

            while(buffer.hasRemaining()) {
                channel.write(buffer);
            }

        }catch(IOException e){
            handleDisconnection(channel);
        }
    }


//---------------------------------------------------------------------------------------


    //NOTIFICA ASINCRONA DEL FINE PARTITA + STATISTICHE (classifica della parita e statistiche personali)
    public void broadcastGameEnd(int gameId){

        JsonObject payload = new JsonObject();
        payload.addProperty("event", "GAME_ENDED");
        payload.addProperty("gameId", gameId);

        //Statistiche della partita conclusa + soluzione
        //Cerco la partita dall'id
        GameManager.HistoricalGameStats stats = gameManager.getHistoricalStats(gameId);
        if(stats != null){
            payload.addProperty("totalParticipants", stats.totalParticipants);
            payload.addProperty("finishedParticipants", stats.finishedParticipants);
            payload.addProperty("wonParticipants", stats.wonParticipants);
            payload.addProperty("averageScore", stats.averageScore);
            
            //Soluzione
            payload.add("groups", gson.toJsonTree(stats.data.groups));
        }


        //Leaderboard del match attuale
        List<User> matchParticipants = new ArrayList<>();
        for(User u : registeredUsers.values()){
            //Aggiungo alla lista gli utenti che hanno partecipato a questa partita
            //(sono sicuro che la history è già aggiornata visto che startNextGame e
            //resetGameStats vengono chiamata prima di broadcastGameEnd)

            if(u.getHistory().get(gameId) != null){
                matchParticipants.add(u);
            }
        }
        
        //Ordina in base al punteggio ottenuto in questa partita (non leaderboard totale)
        matchParticipants.sort((u1, u2) -> Integer.compare(u2.getHistory().get(gameId).score, u1.getHistory().get(gameId).score));
        

        //Array per il payload
        JsonArray matchLeaderArray = new JsonArray();
        //Come per la leaderbaord scorro in ordine decrescente e aggiungo le statistiche
        for(int i = 0; i < matchParticipants.size(); i++){

            User u = matchParticipants.get(i);
            JsonObject entry = new JsonObject();
            entry.addProperty("rank", i + 1);
            entry.addProperty("username", u.getUsername());
            entry.addProperty("score", u.getHistory().get(gameId).score);
            matchLeaderArray.add(entry);
        }

        payload.add("matchLeaderboard", matchLeaderArray);

        
        // Cicliamo tra tutti gli utenti noti attivi e personalizziamo il payload
        //entrySet() mi da un set con coppia chiave valore della hashmap
        for(Map.Entry<String, InetSocketAddress> entry : userUdpAddresses.entrySet()){

            String username = entry.getKey();
            InetSocketAddress address = entry.getValue();
            //Ricavo l'utente
            User user = registeredUsers.get(username);
            
            //Faccio una copia del payload da personalizzare per ogni utente
            JsonObject userPayload = payload.deepCopy();
            
            if(user != null){

                User.GameResult result = user.getHistory().get(gameId);
                //Se ha partecipato
                if(result != null){
                    userPayload.addProperty("participated", true);
                    userPayload.addProperty("myMistakes", result.mistakes);
                    userPayload.addProperty("myScore", result.score);
                    userPayload.addProperty("won", result.won);
                }else{
                    userPayload.addProperty("participated", false);
                }
            }
            
            //Converto l'oggetto in Json e poi in array di byte
            byte[] buffer = gson.toJson(userPayload).getBytes(StandardCharsets.UTF_8);
            
            try{
                //Creo pacchetto udp e lo invio
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length, address);
                udpSocket.send(packet);
            }catch(IOException e){
                System.err.println("[SERVER] Errore invio UDP a " + address);
            }
        }
    }


//---------------------------------------------------------------------------------------


    //INVIA MESSAGGIO A TUTTI I LOGGATI CON PAROLE NUOVE 
    public void broadcastNewGameStart(server.Game newGame){
        //Effettuo subito l'iscrizione alla nuova partita per avere Id coerenti
        for(Map.Entry<SocketChannel, User> entry : activeConnections.entrySet()){
            User user = entry.getValue();
            user.resetGameState(newGame.getGameId());
        }


        //Siccome il messaggio di GAME_ENDED arrivava dopo metto una sleep a quello di NEW_GAME_STARTED
        //Mando un thread che manderà il messaggio e ritorno subito
        new Thread(() ->{
            
            try{
                Thread.sleep(1000);
            }catch(InterruptedException e){
                Thread.currentThread().interrupt();
            }


            JsonObject payload = new JsonObject();
            payload.addProperty("event", "NEW_GAME_STARTED");
            payload.addProperty("message", "Nuova partita iniziata!");
            payload.addProperty("gameId", newGame.getGameId());

            //Aggiorniamo tempo rimanente che nel frattempo è sceso di 1
            payload.addProperty("remainingTimeSeconds", newGame.getRemainingTimeSeconds());
            
            JsonArray wordsArray = gson.toJsonTree(newGame.getAllWords()).getAsJsonArray();
            payload.add("words", wordsArray);

            //Invio a tutti gli utenti
            for(SocketChannel channel: activeConnections.keySet()){
                sendResponse(channel, JsonResponse.success(payload));
            }

        }).start();
    }

}
