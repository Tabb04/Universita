import java.net.*;

//Il programma funziona utilizzando il sito spamhouse, se il quale concateniamo ad un indirizzo IP
//e la getByName() rirotna true, allora è un sito spam.


public class SpamCheck {
    public static final String BLACKHOLE = "zen.spamhouse.org";
    
    public static void main(String[] args) throws UnknownHostException{

        for(String arg: args){
            if(isSpammer(arg)){
                System.out.println(arg + " is a known spammer!");
            }else{
                System.out.println(arg + " looks legitimate!");
            }
        }
    }

    public static boolean isSpammer(String arg){

        try{
            InetAddress address = InetAddress.getByName(arg);
            byte[] quad = address.getAddress();

            String query = BLACKHOLE;

            for(byte octect: quad){ //Scrivo l'indirizzo in unsigned 
                int unsignedByte = octect < 0 ? octect + 256: octect;
                query = unsignedByte + "." + query; 
            }

            InetAddress.getByName(query);
            return true;
    
        }catch(UnknownHostException e){
            return false;
        }
    }

}
