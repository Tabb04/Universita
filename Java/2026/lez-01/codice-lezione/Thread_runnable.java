public class Thread_runnable{
    public static void main(String [] args){
        Thread miothread = new Thread(new Myrunnable());
        miothread.start();
    }
}

class Myrunnable implements Runnable{
    public void run(){
        System.out.println("Eseguendo...");
        System.out.println("Finito!");
    }
}