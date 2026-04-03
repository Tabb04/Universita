public class ContoDeposito implements DepositAccount{
    
    //Senza costruttore
    private double saldo = 0;
    private boolean open = true;

    public double getSaldo(){
        return saldo;
    }

    public boolean isOpen(){
        return open;
    }

    public void versa(double somma){
        if(open){
            saldo += somma;
        }
    }

    public double riscatta(){
        double res = saldo;
        if(open){
            saldo = -1;
            open = false;
        }
        return res;
    }
}
