import java.io.*;

//Per cambiare a buffered basta scommentare e commentare

public class FileCopyNoBuffer{
 
    public static void main(String[] args){
        
        String inFileStr = "photo.png";
        String outFileStr = "photo-new.png";

        long startTime, elapsedTime;
        
        InputStream in;
        OutputStream out;
        
        //Commenta quelli sopra
        //BufferedInputStream in;
        //BufferedOutputStream out;

        int count = 0;

        try{
            in = new FileInputStream(inFileStr);
            out = new FileOutputStream(outFileStr);

            //Commenta quelli sopra
            //in = new BufferedInputStream(new FileInputStream(inFileStr));
            //out = new BufferedOutputStream(new FileOutputStream(outFileStr));


            startTime = System.nanoTime();

            int byteRead;

            while ((byteRead = in.read()) != 1){
                out.write(byteRead);
                count++;
            }

            elapsedTime = System.nanoTime() - startTime;

            System.out.println("Tempo passato: " + (elapsedTime / 1000000.0) + "ms");
            System.out.println("Dimensione file: " + count);
        }catch(IOException e){
            e.printStackTrace();
        }
    }
}