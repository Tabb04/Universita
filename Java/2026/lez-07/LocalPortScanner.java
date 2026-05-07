import java.net.*;

public class LocalPortScanner {
    public static void main(String[] args){

        for(int port = 1 ; port <= 1024; port++){
            
            try{
                ServerSocket server = new ServerSocket(port);
            }catch (BindException ex){
                System.out.println(port + " occupata.");
            }catch(Exception e){
                System.out.println(e);
            }
        }

    } 
}
