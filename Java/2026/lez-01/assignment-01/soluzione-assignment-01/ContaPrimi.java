import java.util.Scanner;


class CountPrimesThread extends Thread{
    int id;
    int MAX; //quantità di numeri da controllare



    public CountPrimesThread(int id, int MAX){
        this.id = id;
        this.MAX = MAX;
    }


    public static int CountPrimes(int min, int max){
        int count = 0;

        for(int i = 0; i <= max; i++){
            if(isPrime(i)){
                count++;
            }
        }
        return count;
    }

    public static boolean isPrime(int x){
        assert x > 1;
        int top = (int)Math.sqrt(x);

        for(int i = 2; i <= top; i++){
            if(x%i == 0){
                return false;
            }
        }
        return true;
    }

    public void run(){
        long startTime = System.currentTimeMillis();
        int count = CountPrimes(2, MAX);

        long elapsedTime = System.currentTimeMillis() - startTime;
        System.out.println("Thread " + id + " counted " + count + " primes in " + (elapsedTime / 1000.0) + " seconds");
    }

}



public class ContaPrimi{
    
    private final static int MAX = 10_000_000; 
    public static void main(String[] args){

        int numberOfThreads = 0;
        Scanner sc = new Scanner(System.in);
        System.out.print("Quanti threads vuoi utilizzare? (range 1-30): ");
        numberOfThreads = sc.nextInt();

        while (numberOfThreads > 30 || numberOfThreads < 0){
            System.out.println("Per favore inserisci un valore tra 1 e 30");
            numberOfThreads = sc.nextInt();
        }

        System.out.println("\nCreo " + numberOfThreads + " threads conta-primi");

        CountPrimesThread[] worker = new CountPrimesThread[numberOfThreads];
        
        for(int i = 0; i<numberOfThreads; i++){
            worker[i] = new CountPrimesThread(i, MAX);  //Passo identificatore e valore
        }

        for(int i = 0; i<numberOfThreads; i++){
            worker[i].start();
        }

        System.out.println("Threads creati e avviati");
        sc.close();

    }
}