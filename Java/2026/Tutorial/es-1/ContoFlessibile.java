public class ContoFlessibile implements BankAccount, DepositAccount {
    
    private double saldo = 0;

    //Metodi comuni alle 2 interfacce
    public double getSaldo(){
        return saldo;
    }
    public void versa(double somma){
        saldo += somma;
    }

    //Metodi BankAccount
    public boolean preleva(double somma){
        if(saldo>=somma){
            saldo -= somma;
            return true;
        }else{
            return false;
        }
    }

    //Metodi DepositAccount
    public boolean isOpen(){
        return true;    //Questo conto non chiude mai
    }
    public double riscatta(){
        double res = saldo;
        saldo = 0;
        return res;
    }
}
