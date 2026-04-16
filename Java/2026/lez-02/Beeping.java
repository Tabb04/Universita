import java.util.concurrent.*;
import java.awt.*;

class BeepClocks implements Runnable{
    public void run(){
        Toolkit.getDefaultToolkit().beep();
    }
}


public class Beeping{
    public static void main(String [] args){
        ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();

        Runnable task = new BeepClocks();

        int initialDelay = 4;

        int perdiodicDelay = 2;

        scheduler.scheduleAtFixedRate(task, initialDelay, perdiodicDelay, TimeUnit.SECONDS);
    }
}