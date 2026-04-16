import java.util.concurrent.*;
import java.util.*;
import java.time.*;


public class FixedDelay{
    private static void periodicShot_FixedDelay(){
        System.out.println("ScheduledExecutor starts at " + LocalDateTime.now());

        Runnable ru = () ->
            {
                System.out.println(Thread.currentThread().getName() + " start: " + Instant.now());
                
                Random r = new Random();
                int dur = 2 + 2*r.nextInt(2);
                
                System.out.println(Thread.currentThread().getName() + " duration: " + dur);
            
                try{
                    TimeUnit.SECONDS.sleep(dur);
                }catch(InterruptedException e){}
                
                System.out.println(Thread.currentThread().getName() + " end:" + Instant.now() + "\n");
            };

            ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
            scheduler.scheduleWithFixedDelay(ru, 2, 3, TimeUnit.SECONDS);


            try{
                scheduler.awaitTermination(30, TimeUnit.SECONDS);
            }catch(InterruptedException ex){
                ex.printStackTrace();
            }

            System.out.println("Shutdown");
            scheduler.shutdown();
    }

    public static void main(String[] args){
        periodicShot_FixedDelay();
    }
}