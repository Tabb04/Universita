class Forth implements Runnable{
    public void run(){
        while(true){

            try{
                Thread.sleep((int)(Math.random()*1000));
            }catch(InterruptedException e){return;}

            System.out.print("**********");
            System.out.flush();
        }
    }
}


class Back implements Runnable{
    public void run(){
        while(true){

            try{
                Thread.sleep((int)(Math.random()*1000));
            }catch(InterruptedException e){return;}

            System.out.print("\b\b\b\b\b\b\b\b\b\b");
            System.out.print("-----------");
            System.out.flush();
        }
    }
}


public class Risorsa_condivisa{
    public static void main(String args[]){
        
        Thread ts = new Thread(new Forth()); 
        ts.start();
        Thread bk = new Thread(new Back()); 
        bk.start();
    }
}