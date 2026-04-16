//Attesa attiva

import java.util.concurrent.*;

class Persona implements Runnable{
    public final int id;

    public final long minDelay = 0; 

    public final long maxDelay = 1000;


    public Persona(int id){
        this.id = id;
    }


    @Override
    public void run(){
        System.out.println("Cliente " + id + " arrivato allo sportello");
        long delay = ThreadLocalRandom.current().nextLong(minDelay, maxDelay);  //generatore casuale più efficiente per evitare di usare lo stesso generatore

        try{
            Thread.sleep(delay);
        }catch(InterruptedException e){
            System.err.println("Interruzione su sleep");
            return;
        }

        
        System.out.println("Cliente " + id + " ha abbandonato l'ufficio");
    }
}


public class UfficioChiusura_attesa_attiva{
    
    public static final int numSportelli = 4;
    public static final int dimCoda = 10;   //Coda davanti agli uffici (seconda stanza)
    
    public static final int numClienti = 200;   //Massimo numero di clienti nella prima stanza


    public static final long queueDelay = 500;      //Ogni quanto provo a far entrare clienti nella coda davanti agli uffici

    public static final long terminationDelay = 5000;   //Tempo di terminazione del pool

    public static final long closingDelay = 60000;      //Tempo di inattività prima della chiusura di uno sportello


    public static void main(String[] args){

        int count = 0;  //Persone servite
        System.out.println("Ufficio aperto!");

        BlockingQueue<Runnable> coda = new LinkedBlockingQueue<Runnable>(); //Coda senza limiti di dimensione

        //Thread con AbortPolicy che solleva un eccezione quando la coda è piena. Voglio che quando tutto è
        //pieno, riprovo a fare submit del task dopo un certo tempo

        ThreadPoolExecutor pool = new ThreadPoolExecutor(numSportelli, 
                                                        numSportelli, 
                                                        closingDelay,
                                                        TimeUnit.SECONDS, 
                                                        new ArrayBlockingQueue<>(dimCoda), 
                                                        new ThreadPoolExecutor.AbortPolicy()
                                                        );


        pool.allowCoreThreadTimeOut(true);  //Mi serve perché in questo modo pure i thread del core possono essere disattivati per inattività


        for(int i = 0; i<numClienti; i++){
            coda.add(new Persona(i));
        }

        while(!coda.isEmpty()){
            Persona p = (Persona) coda.peek();  //peek() restituisce la testa della coda senza rimuoverla
            
            try{
                pool.execute(p);
                coda.poll();    //poll() restituisce e rimuove la testa della coda
                count++;
            }catch(RejectedExecutionException e){
                /*
                Se sono qui significa che la coda davanti agli sportelli (ovvero la coda del pool)
                è piena. Dunque aspetto un certo intervallo di tempo affinchè si svuoti e poi ritento
                */

                System.out.println("Coda sportelli piena, il cliente con id=" + p.id + " resta in attesa");
                try{
                    Thread.sleep(queueDelay);
                }catch(InterruptedException x){
                    System.err.println("Interruzione durante la sleep");
                }
            }
        }

        pool.shutdown();    //Non shutdownNow perché ci potrebbero persone nella seconda stanza in attesa.
        try{
            if(!pool.awaitTermination(terminationDelay, TimeUnit.MILLISECONDS)){
                pool.shutdown();
            }
        }catch (InterruptedException e){
            pool.shutdownNow();
        }

        
        System.out.println("Ufficio chiuso! Persone servite: " + count);
    }


    

}
