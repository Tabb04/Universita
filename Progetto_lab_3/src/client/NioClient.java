package client;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import java.io.IOException;

import java.util.*;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.DatagramChannel;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.nio.charset.StandardCharsets;
import common.JsonRequest;


//GESTISCE RETE LATO CLIENT, USA NIO SELECTOR PER ASCOLTARE I MESSAGGI IN BACKGROUND
public class NioClient implements Runnable{

    private final String serverAddress;
    private final int serverPort;
    private final Gson gson;
    
    private SocketChannel tcpChannel;
    private DatagramChannel udpChannel;     //Per messaggi di fine asincroni
    private Selector selector;
    private boolean running = true;
    

    public NioClient(String serverAddress, int serverPort){
        this.serverAddress = serverAddress;
        this.serverPort = serverPort;
        this.gson = new Gson();
    }

    
//---------------------------------------------------------------------------------------


    //APRE CONNESSIONI E INIZIALIZZA IL SELETTORE
    public void connect() throws IOException{

        selector = Selector.open();
        
        //Connessione TCP verso il server
        tcpChannel = SocketChannel.open(new InetSocketAddress(serverAddress, serverPort));
        tcpChannel.configureBlocking(false);

        //Inserisco con buffer per i messaggi in ingresso
        tcpChannel.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(8192));
        
        //Canale UDP per ricevere i broadcast asincroni
        udpChannel = DatagramChannel.open();
        udpChannel.configureBlocking(false);
        udpChannel.bind(new InetSocketAddress(0));  //Porta scelta dall'os
        udpChannel.register(selector, SelectionKey.OP_READ, ByteBuffer.allocate(4096));
    }
    

//---------------------------------------------------------------------------------------


    //RESTITUISCE LA PORTA UDP LOCALE USATA DAL CLIENT (comunicata a login)
    public int getUdpPort(){
        try{
            return ((InetSocketAddress) udpChannel.getLocalAddress()).getPort();
        
        }catch(IOException e){
            return -1;
        }
    }


//---------------------------------------------------------------------------------------


    //TRASFORMA LA RICHIESTA IN JSON E LA INVIA
    public void sendRequest(JsonRequest request){
        try{
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

    
//---------------------------------------------------------------------------------------


    //LEGGE I DATI IN ARRIVO
    public void run(){
        try{
            while(running && !Thread.interrupted()){

                selector.select();
                Iterator<SelectionKey> keys = selector.selectedKeys().iterator();
                
                while(keys.hasNext()){
                    SelectionKey key = keys.next();
                    keys.remove();
                    
                    if(!key.isValid()){
                        continue;
                    }
                    
                    if(key.isReadable()){

                        //Su TCP
                        if(key.channel() == tcpChannel){
                            readTcp(key);
                        
                        //Su UDP
                        }else if(key.channel() == udpChannel){
                            readUdp(key);
                        }
                    }
                }
            }
        }catch(IOException e){
            if(running){
                System.err.println("Errore di rete NioClient.");
            }
        }
    }
    
//---------------------------------------------------------------------------------------
    

    //LEGGE MESSAGGIO TCP E LO STAMPA
    private void readTcp(SelectionKey key) throws IOException{

        SocketChannel channel = (SocketChannel) key.channel();
        ByteBuffer buffer = (ByteBuffer) key.attachment();
        
        int bytesRead = channel.read(buffer);
        if(bytesRead == -1){

            //-1 è EOF praticamente
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
        
        if(!msg.isEmpty()){
            System.out.println("\n[SERVER] -> " + msg);
            System.out.print("> ");     //Ripristino il prompt sulla cli
        }
    }

    
//---------------------------------------------------------------------------------------


    //LEGGE MESSAGGIO BROADCAST UDP
    private void readUdp(SelectionKey key) throws IOException{
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
