import java.util.*;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ExampleFixed_threadpool_prof{
    public static void main(String[] args){
        
        //Creo il pool fixed
        ExecutorService servizio = Executors.newFixedThreadPool(10);

        //Spedisco i task definiti sotto
        for(int i = 0; i<100; i++){
            servizio.execute(new Task(i));
        }

        System.out.println("Thread name: " + Thread.currentThread().getName());
        
    }
}





class Task implements Runnable{
    
    private int name;

    public Task(int name){
        this.name = name;
    }

    public void run(){
        try{
            Long duration = (long)(Math.random()*10);
            System.out.printf("%s: Task %s: Inizia task di durata %d secondi\n", Thread.currentThread().getName(), name, duration);
            Thread.sleep(duration);
        }catch(InterruptedException e){
            e.printStackTrace();
        }
        
        System.out.printf("%s: Task finito %s\n", Thread.currentThread().getName(), name);
    }


}
