package common;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;


//CLASSE PER LEGGERE LE CONFIGURAZIONE DA FILE .properties
public class ConfigReader{
    private final Properties properties;


    public ConfigReader(String filePath){

        //Popolato come una hashtable di un file
        properties = new Properties();

        //Faccio la try with resources
        try(InputStream input = new FileInputStream(filePath)){
            properties.load(input);

        }catch(IOException ex){
            System.err.println("Errore nel caricamento del file di configurazone ("+ filePath +"): "+ ex.getMessage());
        }
    }


//---------------------------------------------------------------------------------------


    //COME LA FUNZIONE DI LIBRERIA RESTITUISCE IL VALORE ASSOCIATO ALLA CHIAVE, ALTRIMENTI DEFAULT
    public String getProperty(String key, String defaultValue){
        return properties.getProperty(key, defaultValue);
    }


//---------------------------------------------------------------------------------------


    //UGUALE ALLA getProperty MA PER INTERI (funzione di default da una stringa)
    public int getIntProperty(String key, int defaultValue){
        String value = properties.getProperty(key);
        if(value != null){
            try{
                return Integer.parseInt(value);
            }catch(NumberFormatException e){
                System.err.println("Errore di parsing per la property " +key + " uso valore default: " + defaultValue);
            }
        }
        return defaultValue;
    }
}
