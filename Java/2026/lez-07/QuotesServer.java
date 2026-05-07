import java.net.*;
import java.io.*;
import java.util.Random;
import java.util.random.*;

public class QuotesServer {
    public static void main(String[] args){

        int port = 2017;    //porta random non tra le riservate (0-1023)

        //Alcune frasi esempio
 
        String[] quotes = {
            "Posso resistere a tutto tranne che alla tentazione - Oscar Wilde",
            "L’istruzione è ciò che resta dopo che si è dimenticato ciò che si è imparato a scuola - Albert Einstein",
            "Sii il cambiamento che vuoi vedere nel mondo - Mahatma Gandhi ",
            "Il successo non è definitivo, il fallimento non è fatale: ciò che conta è il coraggio di continuare - Winston Churchill",
            "Scegli un lavoro che ami, e non dovrai lavorare neppure un giorno in vita tua - Confucio"
        };

        /*
            Questi sono "Try With Resources".
            Metto nelle parentesi tonde del try delle risorse (file, connessioni di rete, console ecc).
            
            Qualsiasi sia il modo in cui esco dalla try-catch (finito il blocco o per eccezione CHIUDE LA RISORSA IN AUTOMATICO)    
         */


        try(ServerSocket serverSocket = new ServerSocket(port)){
            System.out.println("Quote of the day attivo sulla porta " + port);
        
            while (true) {
                try(Socket clientSocket = serverSocket.accept()){   //Si ferma qui ed aspetta, quando un client si connette restituisce un nuovo socket
                    System.out.println("Connessione da: " + clientSocket.getInetAddress());

                    try(PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)){
                        String quote = quotes[new Random().nextInt(quotes.length)];
                        out.println(quote);
                        System.out.println("Inviata: " + quote);
                    }catch(IOException e){
                        System.err.println("Errore nella scrittura: " + e.getMessage());
                    }
                }catch(IOException ex){
                    System.err.println("Errore con un client: " + ex.getMessage());

                }
            }
        }catch(IOException ex1){
            System.err.println("Errore nell'avvio del server: " + ex1.getMessage());

        }
    }
}
