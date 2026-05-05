import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class EsempioExecutor {
    public static void main(String[] args) {
        
        // 1. Definisci il task (esattamente come prima, usando Runnable o la sua evoluzione Callable)
        Runnable mioTask = () -> {
            System.out.println("Lavoro eseguito da: " + Thread.currentThread().getName());
        };

        // 2. "Assumi il Manager" creando un Thread Pool (es. 3 operai fissi)
        ExecutorService manager = Executors.newFixedThreadPool(3);

        // 3. Consegni i task al manager (non fai più start() manualmente!)
        manager.submit(mioTask);
        manager.submit(mioTask);
        manager.submit(mioTask);
        manager.submit(mioTask); // Questo 4° task aspetterà in automatico che un operaio si liberi

        // 4. Dici al manager di non accettare più task e chiudere quando ha finito
        manager.shutdown();
    }
}