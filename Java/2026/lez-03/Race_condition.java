//FAccio 2 thread che in race condition aumentano e diminuiscono un valore condiviso

//Per vedere le due versioni scommentare il metodo giusto nella run del thread

class Cell{
    private long value;

    public Cell(int v){
        this.value = v;
    }

    public void update(int v){
        this.value+=v;
    }

    public synchronized void update_safe(int v){
        this.value+=v;
    }

    public long get(){
        return this.value;
    }

}


class Counter implements Runnable{
    
    public int ticks;
    private Cell cell;
    private int delta;
    private int maxTicks;

    public Counter(Cell cell, int delta, int maxTicks){

        this.cell = cell;
        this.delta = delta;
        this.maxTicks = maxTicks;
    }

    public void run(){
        ticks = 0;
        
        while(ticks < maxTicks){
            //cell.update(delta);
            cell.update_safe(delta);
            ticks++;
        }
    }
}



public class Race_condition{
    public static void main (String[] args){
        int MAX_TICKS = 1_000_000;
        
        Cell cell = new Cell(0);
        Counter up = new Counter(cell, 1, MAX_TICKS);
        Counter down = new Counter(cell, -1, MAX_TICKS);

        Thread upWorker = new Thread(up);
        Thread downWorker = new Thread(down);

        upWorker.start(); downWorker.start();

        try{
            upWorker.join();
            downWorker.join();
        }catch(Exception e){}

        System.out.println("\nValore cell: " + cell.get());
        System.out.flush();
    }
}
