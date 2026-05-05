import java.net.*;
import java.io.*;

//Facendo così faccio 1024 interrogazioni al DNS

public class LowPortScanner {
    
    public static void main(String[] args){

        String host = args.length > 0 ? args[0] : "localhost";

        for(int i = 0; i<1024; i++){
            try{
                Socket s = new Socket(host, i); //Host host e porta i
                System.out.println("C'è un server sulla porta " + i + " di " + host);
                s.close();
            
            }catch(UnknownHostException e){
                System.err.println(e);
                break;
            }catch (IOException ex){
                //non ho trovato niente su questa porta
            }
        }
    }
}
