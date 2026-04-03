//Secondo mondo di implementare un thread è di ereditare dalla classe thread
//e fare overriding del metodo run()
public class ExtendingThread{


    public static class MyThread extends Thread{
        public void run(){
            System.out.println("My thread running");
            System.out.println("My thread finished");
        }
    }

    public static void main(String[] args){
        MyThread miothread = new MyThread();
        miothread.start();
    }
}
