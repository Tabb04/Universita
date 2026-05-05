
public class ThreadRunnable_inner {
    
    //Faccio una inner class che sarà ciò che passo come parametro al mio thread 
    public static class MyRunnable implements Runnable {
        public void run() {
            System.out.println("MyRunnable running");
            System.out.println("MyRunnable finished");
        }
    }

    public static void main(String[] args) {
        Thread thread = new Thread(new MyRunnable());
        thread.start();
    }
}