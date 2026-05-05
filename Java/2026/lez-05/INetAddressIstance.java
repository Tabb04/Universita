import java.util.Arrays;
import java.net.*;
import java.io.*;


public class INetAddressIstance {
    public static void main(String[] args) throws IOException{

        final String nome = "www.google.com"; 
        InetAddress ia1 = InetAddress.getByName(nome);

        byte [] address = ia1.getAddress();

        for(int i = 0; i<address.length; i++){
            System.out.print(address[i]);
            System.out.println(" -> Sommando a 256 = " + (256 + address[i]));
        }

        System.out.println(ia1.getHostAddress());
        System.out.println(ia1.getHostName());
        System.out.println(ia1.isReachable(1000));
        System.out.println(ia1.isLoopbackAddress());
        System.out.println(ia1.isMulticastAddress());
    }
}


