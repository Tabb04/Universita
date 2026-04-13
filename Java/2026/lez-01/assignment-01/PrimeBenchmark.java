import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;


public class PrimeBenchmark{
    
    public static void main(String [] args){
        Scanner scanner = new Scanner(System.in);

        System.out.println("Inserisci il numero di task/thread da eseguire");
        int numeroTask = scanner.nextInt();

        int coreDisponibili = Runtime.getRuntime().availableProcessors();
        System.out.println("Il sistema ha " + coreDisponibili + " core disponibili");

        System.out.println("--------------------------------------");
        System.out.println("Test 1: Creazine manuale di " + numeroTask + " thread");
        eseguiConThreadManuali(numeroTask);


        System.out.println("--------------------------------------");
        System.out.println("Test 2: Threadpool di dimensione " + coreDisponibili + " (core disponibili)");
        eseguiConThreadpool(numeroTask, coreDisponibili);

        scanner.close();
    }

    private static void eseguiConThreadManuali(int numeroTask){
        List<Thread> threads = new ArrayList<>();

        long tStart = System.currentTimeMillis();

        for (int i = 0; i<numeroTask; i++){
            Thread t = new Thread(() -> contaPrimi()); //funzione da passare
            threads.add(t);
            t.start();
        }

        for(Thread t: threads){
            try{
                t.join();
            }catch(InterruptedException e){
                e.printStackTrace();
            }
        }

        long tEnd = System.currentTimeMillis();
        System.out.println("Tempo passato (ms): " + (tEnd - tStart));
    }


    private static void eseguiConThreadpool(int numeroTask, int poolSize){  //poolSize sono i core disponibili

        ExecutorService esecutori = Executors.newFixedThreadPool(poolSize);
        List<Future<?>> futures = new ArrayList<>();


        long tStart = System.currentTimeMillis();

        for(int i = 0; i<numeroTask; i++){
            futures.add(esecutori.submit(()->contaPrimi()));//funzione da mandare
        }

        for (Future<?> future : futures){
            try{
                future.get();    //Bloccante finchè il singolo task non finisce
            }catch(Exception e){
                e.printStackTrace();
            }
        }

        esecutori.shutdown();

        long tEnd = System.currentTimeMillis();
        System.out.println("Tempo passato (ms): " + (tEnd - tStart));
    }

    private static void contaPrimi(){
        int count = 0;
        int max = 10000000;
    
        for(int i = 2; i<max; i++){
            boolean isPrime = true;
        
            for(int j = 2; j*j <= i; j++){
                if(i%j == 0){
                    isPrime = false;
                    break;
                }
            }
            if(isPrime){
                count++;
            }
        
        }    
    
    }


}