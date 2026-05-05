public class Runnable_anonimo{
    public static void main(String[] args){
        Runnable miorunnable = new Runnable() {
            public void run(){
                System.out.println("Eseguendo...");
                System.out.println("Finito!");
            }
        };

        Thread miothread = new Thread(miorunnable);
        miothread.start();
    }
}