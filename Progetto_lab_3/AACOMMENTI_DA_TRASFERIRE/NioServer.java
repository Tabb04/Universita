package server;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
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
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import common.JsonRequest;
import common.JsonResponse;
import server.GameManager;
import server.User;


//SERVER NIO
//Accetta e smista connessioni. Usa Threadpool per elaborazione


public class NioServer{
    private final int port;
    private final ExecutorService workerPool;   //Ho optato per un cached
    private GameManager gameManager;            // ???? Gestore del ciclo di gioco
    private final Gson gson;
    private Selector selector;
    private ServerSocketChannel serverChannel;
    private DatagramSocket udpSocket;
    
    //Mappa globale per salvare gli utenti registrati
    private final ConcurrentHashMap<String, User> registeredUsers = new ConcurrentHashMap<>();
    
    //Mappa per associare un SocketChannel a un utente loggato
    private final ConcurrentHashMap<SocketChannel, User> activeConnections = new ConcurrentHashMap<>();
    
    // Mappa per associare un utente loggato alla sua porta UDP (per le notifiche asincrone)
    private final ConcurrentHashMap<String, InetSocketAddress> userUdpAddresses = new ConcurrentHashMap<>();

    public NioServer(int port) {
        this.port = port;
        this.workerPool = Executors.newCachedThreadPool();
        this.gson = new Gson();
    }

    public void setGameManager(GameManager gameManager) {
        this.gameManager = gameManager;
    }
    
    public GameManager getGameManager() { return this.gameManager; }
    public ConcurrentHashMap<String, User> getRegisteredUsers() { return this.registeredUsers; }
    public ConcurrentHashMap<SocketChannel, User> getActiveConnections() { return this.activeConnections; }
    public ConcurrentHashMap<String, InetSocketAddress> getUserUdpAddresses() { return this.userUdpAddresses; }

    /**
     * Avvia il server NIO e il listener sulla porta.
     */
    public void start() throws IOException {
        selector = Selector.open();
        
        // Configuriamo il canale del server come non bloccante
        serverChannel = ServerSocketChannel.open();
        serverChannel.bind(new InetSocketAddress(port));
        serverChannel.configureBlocking(false);
        serverChannel.register(selector, SelectionKey.OP_ACCEPT);
        
        // Apriamo un socket UDP standard per l'invio delle notifiche fire-and-forget.
        udpSocket = new DatagramSocket();
        
        System.out.println("[SERVER] In ascolto sulla porta TCP " + port);

        // Loop principale del Selettore
        while (!Thread.interrupted()) {
            selector.select(); // Si blocca finché non c'è attività
            Set<SelectionKey> selectedKeys = selector.selectedKeys();
            Iterator<SelectionKey> iter = selectedKeys.iterator();

            while (iter.hasNext()) {
                SelectionKey key = iter.next();
                iter.remove();

                if (!key.isValid()) continue;

                if (key.isAcceptable()) {
                    acceptConnection(key); // Nuova connessione TCP in entrata
                } else if (key.isReadable()) {
                    readRequest(key); // Dati disponibili da leggere
                }
            }
        }
    }

    /**
     * Accetta la nuova connessione TCP dal client.
     */
    private void acceptConnection(SelectionKey key) throws IOException {
        ServerSocketChannel ssc = (ServerSocketChannel) key.channel();
        SocketChannel clientChannel = ssc.accept();
        clientChannel.configureBlocking(false);
        // Ogni connessione registra il proprio buffer per leggere i dati JSON
        clientChannel.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(8192));
        System.out.println("[SERVER] Nuova connessione TCP da " + clientChannel.getRemoteAddress());
    }

    /**
     * Legge la stringa JSON dal canale del client e la passa al worker.
     */
    private void readRequest(SelectionKey key) {
        SocketChannel clientChannel = (SocketChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();

        try {
            int bytesRead = clientChannel.read(buffer);
            if (bytesRead == -1) {
                // Il client ha chiuso bruscamente la connessione
                handleDisconnection(clientChannel);
                return;
            }

            buffer.flip();
            byte[] data = new byte[buffer.remaining()];
            buffer.get(data);
            String message = new String(data, StandardCharsets.UTF_8).trim();
            buffer.compact(); // Prepara per le letture successive (se ci sono frammentazioni)

            if (!message.isEmpty()) {
                // Sottomette l'elaborazione al pool in modo asincrono
                workerPool.submit(() -> processMessage(clientChannel, message));
            }
        } catch (IOException e) {
            handleDisconnection(clientChannel);
        }
    }

    /**
     * Rimuove l'utente dalle mappe attive se si disconnette.
     */
    private void handleDisconnection(SocketChannel clientChannel) {
        try {
            User u = activeConnections.remove(clientChannel);
            if (u != null) {
                userUdpAddresses.remove(u.getUsername());
                System.out.println("[SERVER] Utente disconnesso: " + u.getUsername());
            } else {
                System.out.println("[SERVER] Client anonimo disconnesso");
            }
            clientChannel.close();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    /**
     * Eseguito dal thread worker per calcolare la risposta corretta.
     */
    private void processMessage(SocketChannel clientChannel, String message) {
        try {
            // Tenta il parsing della richiesta (il messaggio deve finire con un marker o essere un JSON valido,
            // per semplicità qui assumiamo che il client invii JSON completi ad ogni invio).
            JsonRequest request = gson.fromJson(message, JsonRequest.class);
            
            // Smistamento logica al processore
            JsonResponse response = CommandProcessor.process(request, clientChannel, this);

            // Invia il risultato indietro
            sendResponse(clientChannel, response);
            
        } catch (Exception e) {
            e.printStackTrace();
            JsonResponse err = JsonResponse.error(400, "Richiesta malformata o errore server.");
            sendResponse(clientChannel, err);
        }
    }

    /**
     * Invia in modo sincrono la stringa JSON sul Socket TCP del client.
     */
    private void sendResponse(SocketChannel channel, JsonResponse response) {
        try {
            // Aggiungiamo un \n al termine per aiutare il client a delimitare i messaggi
            String jsonStr = gson.toJson(response) + "\n";
            ByteBuffer buffer = ByteBuffer.wrap(jsonStr.getBytes(StandardCharsets.UTF_8));
            while (buffer.hasRemaining()) {
                channel.write(buffer);
            }
        } catch (IOException e) {
            handleDisconnection(channel);
        }
    }

    /**
     * Invia un pacchetto UDP a tutti i client loggati per avvisarli del termine della partita.
     */
    public void broadcastGameEnd(int gameId) {
        JsonObject payload = new JsonObject();
        payload.addProperty("event", "GAME_ENDED");
        payload.addProperty("gameId", gameId);
        
        String message = gson.toJson(payload);
        byte[] buffer = message.getBytes(StandardCharsets.UTF_8);

        // Cicliamo tra tutti gli utenti noti attivi
        for (InetSocketAddress address : userUdpAddresses.values()) {
            try {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length, address);
                udpSocket.send(packet);
            } catch (IOException e) {
                System.err.println("[SERVER] Errore UDP verso " + address);
            }
        }
    }
}
