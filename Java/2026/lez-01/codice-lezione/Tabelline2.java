public class Tabelline2{

    public static void main(String[] args){
        for(int i = 0; i<11; i++){
            Thread thread = new Thread(new Tabellina(i));
            thread.start();
            System.out.println(" Avviata tabellina del " + i);
        }
    }
}


class Tabellina implements Runnable{
    
    int number;

    public Tabellina(int number){
        this.number = number;
    }

    public void run(){
        for(int i = 0; i<11; i++){
            System.out.print(Thread.currentThread().getName());
            System.out.printf(": Tabellina del %d = %d * %d = %d\n", number, number, i, number * i );
        }
    }
}