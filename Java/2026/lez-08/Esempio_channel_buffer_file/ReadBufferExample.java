import java.nio.*;
import java.nio.channels.FileChannel;
import java.io.*;

public class ReadBufferExample {
    public static void main(String[] args){

        try(FileInputStream fis = new FileInputStream("output.txt");
            FileChannel channel = fis.getChannel()){

                //Sopra stesso procedimento della scrittura
                //1. Creo un buffer da 64 bytes
                ByteBuffer buffer = ByteBuffer.allocate(64);

                //2. Leggiamo i dati dal file (il canale scrive nel buffer)
                int bytesRead = channel.read(buffer);   //bytesRead = numero di byte letti dal canale

                while (bytesRead != -1) {
                    System.out.println("Letti " + bytesRead + " byte dal file");
                
                    //3. Preparo il buffer per la lettura
                    buffer.flip();

                    //4. Leggo i dati dal buffer
                    while (buffer.hasRemaining()) {
                        System.out.print((char)buffer.get());
                    }

                    //5. Ripulisco il buffer per il prossimo ciclo
                    buffer.clear();

                    //6. Leggiamo altri dati (se il file è più lungo)
                    bytesRead = channel.read(buffer);
                }
            }catch(Exception e){
                e.printStackTrace();
            }
        System.out.println();
    }    
}
