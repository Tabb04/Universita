

public class Banca{
    public static void main(String [] args){    
        //Posso usare main perché è statico quindi non devo creare un oggetto.
        
        BankAccount conto1 = new ContoCorrente(1000);
        
        BankAccount conto2 = new ContoLimitato(200, 10);

        //BankAccount è tipo apparente (o statico) dei conti
        //ContoCorrente e ContoLimitato sono i tipi effettivi (o dinamici) dei conti

        if(conto1.preleva(700)){
            conto2.versa(700);
        }


        System.out.println("Saldo primo conto: " + conto1.getSaldo());
        System.out.println("Saldo secondo conto: " + conto2.getSaldo());

    }
}