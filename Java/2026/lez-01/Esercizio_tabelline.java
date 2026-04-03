
/*
scrivere un programma che stampi le tabelline moltiplicative dall' 1 al 10
●si attivino 10 threads
●ogni numero n, 1 =< n =< 10, viene passato ad un thread diverso
●il task assegnato ad ogni thread consiste nello stampare la tabellina
corrispondente al numero che gli è stato passato come parametro

 */

public class Esercizio_tabelline{ 
    public static void main(String[] args){
        System.out.println("Inizio calcolo delle tabelline...");

        for(int i = 0; i<=10; i++){

            //Creo un nuovo task passando il numero della tabellina
            Calculator task = new Calculator(i);

            Thread thread = new Thread(task);

            thread.setName("Thread tabellina-" + i);
            
            thread.start();
        }
    }
}




class Calculator implements Runnable{
    private int number;

    public Calculator(int number){
        this.number = number;
    }

    public void run(){
        for(int i = 0; i<=10; i++){
            System.out.printf("%s: %d * %d = %d\n", Thread.currentThread().getName(), number, i, i*number);
        }
    }
}