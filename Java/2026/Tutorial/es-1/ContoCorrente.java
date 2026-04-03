
public class ContoCorrente implements BankAccount{
    private double saldo;   //Saldo d'istanza
    private int numero; //Numero di conto privato per ogni istanza
    private static int numeroUltimoConto = 1000;    //Contatore statico per i numeri di conto univoci
    private static double tasso = 0.02;     //Tasso di interesse statico

    public ContoCorrente(double saldoIniziale){
        saldo = saldoIniziale;
        numeroUltimoConto++;
        numero = numeroUltimoConto;
    }

    public void versa(double somma){
        saldo += somma;
        System.out.println("Versati: " + somma + " euro");
    }

    public double getSaldo(){
        return saldo;
    }

    public int getNumero(){
        return numero;
    }

    public static double getTasso(){    //Metodo statico usa valori statici (duh!)
        return tasso;
    }

    public void maturaInteressi(){
        saldo += saldo*tasso;
    }

    
    public boolean preleva(double somma){
        if(saldo >= somma){
            saldo -= somma;
            System.out.println("Prelevati: " + somma + " euro");
            return true;        
        }else{
            return false;
        }

    }
}


