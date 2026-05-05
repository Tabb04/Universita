import java.net.*;
import java.io.*;

public class QuoteOfTheDay {
    public static void main (String[] args){

        String host = "djxmmx.net";
        int port = 17;
        Socket socket = null;


        try{
            socket = new Socket(host, port);

            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            System.out.print("Connesso a djxmmx. Ecco la tua citazione del giorno: ");

            String line;
            while((line = in.readLine()) != null){
                System.out.println(line);
            }
            System.out.println("Fine della citazione. Connessione chiusa dal server");
        }catch(IOException e){
            e.printStackTrace();
        }

        try{
            socket.close();
        }catch(IOException ex){
            ex.printStackTrace();
        }
    }
}
