public class Test{
    public static void main(String [] args){
        int[] numeri;

        numeri = new int[10];
        //numeri = {5, 3, 2, 6}; altrimenti uno per uno
        for (int i = 0; i < numeri.length; i++){
            numeri[i] = i;
        }

        for (int n: numeri){
            System.out.println(numeri[n]);
        }
        
        //System.out.println();
    }

    public int fattoriale(int n){
        int res = 1;
        if (n == 0){
            return res;
        }else{
            return n * fattoriale(n-1);
        }
    }
}

