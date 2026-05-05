import java.net.*;

public class Find_IP {
    public static void main(String[] args){
        final String sito = "www.unipi.it";
        try{
            InetAddress address = InetAddress.getByName(sito);
            System.out.println("Indirizzo di "+sito + ": " + address);  
        }catch(UnknownHostException ex){
            System.out.println("Could not find host " + sito);
        }

        //trovo il mio indirizzo locale
        try{
            InetAddress myaddress = InetAddress.getLocalHost();
            System.out.println("Indirizzo locale: " + myaddress);
        }catch(UnknownHostException ex1){
            System.out.println("Errore nel cercare indirizzo locale");
        }
    }   
    
}
