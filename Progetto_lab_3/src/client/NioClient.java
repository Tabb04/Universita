package client;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.DatagramChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import common.JsonRequest;

/**
 * Gestisce la rete lato client (TCP e UDP).
 * Usa NIO Selector per ascoltare i messaggi in background.
 */
public class NioClient implements Runnable {
    private final String serverAddress;
    private final int serverPort;
    private final Gson gson;
    
    private SocketChannel tcpChannel;
    private DatagramChannel udpChannel;
    private Selector selector;
    private boolean running = true;
    
    public NioClient(String serverAddress, int serverPort) {
        this.serverAddress = serverAddress;
        this.serverPort = serverPort;
        this.gson = new Gson();
    }
    
    /**
     * Apre le connessioni e inizializza il selettore.
     */
    public void connect() throws IOException {
        selector = Selector.open();
        
        // Connessione TCP verso il server
        tcpChannel = SocketChannel.open(new InetSocketAddress(serverAddress, serverPort));
        tcpChannel.configureBlocking(false);
        // Buffer per i messaggi in ingresso (sincroni TCP)
        tcpChannel.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(8192));
        
        // Canale UDP per ricevere i broadcast asincroni (fine partita)
        udpChannel = DatagramChannel.open();
        udpChannel.configureBlocking(false);
        udpChannel.bind(new InetSocketAddress(0)); // Porta effimera
        udpChannel.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(4096));
    }
    
    /**
     * Restituisce la porta UDP locale usata dal client, per comunicarla al server.
     */
    public int getUdpPort() {
        try {
            return ((InetSocketAddress) udpChannel.getLocalAddress()).getPort();
        } catch (IOException e) {
            return -1;
        }
    }
    
    /**
     * Trasforma la richiesta in JSON e la invia.
     */
    public void sendRequest(JsonRequest request) {
        try {
            String jsonStr = gson.toJson(request) + "\n";
            ByteBuffer buffer = ByteBuffer.wrap(jsonStr.getBytes(StandardCharsets.UTF_8));
            while (buffer.hasRemaining()) {
                tcpChannel.write(buffer);
            }
        } catch (IOException e) {
            System.err.println("Errore di connessione TCP.");
            running = false;
        }
    }
    
    /**
     * Thread in background che legge i dati in arrivo senza bloccare la Console.
     */
    @Override
    public void run() {
        try {
            while (running && !Thread.interrupted()) {
                selector.select(); // Attende eventi
                Iterator<SelectionKey> keys = selector.selectedKeys().iterator();
                
                while (keys.hasNext()) {
                    SelectionKey key = keys.next();
                    keys.remove();
                    
                    if (!key.isValid()) continue;
                    
                    if (key.isReadable()) {
                        if (key.channel() == tcpChannel) {
                            readTcp(key); // Dati dal server in risposta a un comando
                        } else if (key.channel() == udpChannel) {
                            readUdp(key); // Notifica asincrona (es. fine tempo)
                        }
                    }
                }
            }
        } catch (IOException e) {
            if (running) System.err.println("Errore di rete NioClient.");
        }
    }
    
    /**
     * Legge la risposta TCP e la stampa a schermo.
     */
    private void readTcp(SelectionKey key) throws IOException {
        SocketChannel channel = (SocketChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();
        
        int bytesRead = channel.read(buffer);
        if (bytesRead == -1) {
            System.err.println("\nConnessione chiusa dal server.");
            running = false;
            System.exit(1);
            return;
        }
        
        buffer.flip();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data);
        String msg = new String(data, StandardCharsets.UTF_8).trim();
        buffer.compact();
        
        if (!msg.isEmpty()) {
            System.out.println("\n[SERVER] -> " + msg);
            System.out.print("> "); // Ripristina il prompt della CLI
        }
    }
    
    /**
     * Legge il broadcast UDP.
     */
    private void readUdp(SelectionKey key) throws IOException {
        DatagramChannel channel = (DatagramChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();

        buffer.clear();
        channel.receive(buffer);
        buffer.flip();

        byte[] data = new byte[buffer.remaining()];
        buffer.get(data);
        String msg = new String(data, StandardCharsets.UTF_8).trim();

        // Mostriamo esplicitamente che si tratta di un broadcast
        System.out.println("\n[BROADCAST UDP] -> " + msg);

        System.out.print("> ");
    }
}
