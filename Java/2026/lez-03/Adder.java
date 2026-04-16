import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;


//Faccio la somma di tutti i valori da 1 a 10.
//Suddivido in 5 somme (1,2); (3,4); ....
//Eseguo 5 thread che fanno la somma delle coppie e restituiscono il risultato poi sommo le somme nel main thread

class Calculator implements Callable<Integer>{  //Siccome è callable, indico che restituisce un "Integer"
    
    private int a;
    private int b;

    public Calculator(int a, int b){
        this.a = a;
        this.b = b;
    }


    public Integer call() throws Exception{
        Thread.sleep((long)(Math.random()*15_000));
        return a + b;
    }
}



public class Adder{
    public static void main(String[] args) throws ExecutionException,InterruptedException{
        ExecutorService executor = Executors.newFixedThreadPool(5);

        List<Future<Integer>> list = new ArrayList<>();

        for(int i = 1; i<11; i+=2){
            Calculator c = new Calculator(i, i+1);
            list.add(executor.submit(c));
        }
        int s = 0;

        for(Future<Integer> f : list){
            try{
                System.out.println(f.get());
                s+=f.get();
            }catch(Exception e){}
        }

        System.out.println("La somma è: " + s);
        executor.shutdown();
    }
}