import java.io.*;

//Elenca tutti i file nella directory specificata che hanno estenzione ".java"


public class ListFiles {
    public static void main(String[] args){

        File dir = new File(".");

        if(dir.isDirectory()){
            String[] files = dir.list();

            for(String file: files){
                if(file.endsWith(".java")){
                    System.out.println(file);
                }
            }
        }
    }    
}
