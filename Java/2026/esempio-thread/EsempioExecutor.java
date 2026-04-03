import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;


public class EsempioExecutor{
    public static void main(String [] args){

        //1.Creiamo il nostro Pool con 2 lavoratori
        //Invece di fare "new" uso Executors che è una factory
        //Dico poi che il nostro Pool ha un numero fisso di 2 threads
        ExecutorService executor = Executors.newFixedThreadPool(2);
    

        //Creiamo una lista per conservare le nostre "promesse" (Future)
        
        /*
        Quando invio un task con promesseDiRisultato.add() mi restituisce subito
        un tipo
        */
        
        List<Future<String>> promesseDiRisultato = new ArrayList<>();
    
    
        System.out.println("Inizio invio dei task all'Executor");

        for (int i = 0; i <= 3; i++){
            final int numeroTask = i;

            //taskDiDownload deve contenere un oggetto che può essere chiamato con "call()", non accetta altri parametri e restituisce una stringa
            Callable<String> taskDiDownload = () -> {
                System.out.println("-> Thread [" + Thread.currentThread().getName() + "] Inizia il download simulato numero " + numeroTask);
                Thread.sleep(2000);
                return "Download " + numeroTask + " completato con successo!";
            };

            //Consegno questo task all'executor
            //Non mi blocco qui, restituisce uno scontrino (future)
            Future<String> scontrino = executor.submit(taskDiDownload);
            //Lo aggiungo alla coda di scontrini
            promesseDiRisultato.add(scontrino);
        }
    
        System.out.println("Tutti task inviati");

        for(Future<String> scontrino : promesseDiRisultato){
            try{
                //Se get non da risultato il main thread aspetta
                String risultato = scontrino.get();
                System.out.println("Risultato ricevuto" + risultato);
            }catch(Exception e){
                System.out.println("Errore" + e.getMessage());
            }
        }
    

        //Devo spengere l'executor altrimenti i thread rimangono in attesa di lavoro
        executor.shutdown();
        System.out.println("Executor spento, uscita...");
    }
}