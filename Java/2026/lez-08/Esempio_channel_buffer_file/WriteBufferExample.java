import java.nio.*;
import java.nio.channels.FileChannel;
import java.io.*;

public class WriteBufferExample {
    public static void main(String[] args){

        String messaggio = "Hello Nio!";

        //1. Creo uno stream per il file di testo
        //2. Associo un channel a quello stream.
    
        try(FileOutputStream fos = new FileOutputStream("output.txt");
            FileChannel channel = fos.getChannel()){

                //3. Creo un buffer
                ByteBuffer buffer = ByteBuffer.allocate(64);

                //4. Scrivo dati nel buffer
                buffer.put(messaggio.getBytes());

                //5. Preparo il buffer alla scrittura
                buffer.flip();

                //6. Faccio leggere al channel i dati dal buffer che li mette nello stream
                channel.write(buffer);
                
                System.out.println("Dati scritti con successo");
            
            }catch(Exception e){
                e.printStackTrace();
            }
    }
}
