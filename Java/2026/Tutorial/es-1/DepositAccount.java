
/*
Conto di deposito: si può versare anche più volte e poi
chiudere una volta per tutte riscattando il saldo
*/

public interface DepositAccount{
    
    public double getSaldo();
    public boolean isOpen();    //Dice se il conto è aperto
    public void versa(double somma);
    public double riscatta();  //Riscatta e chiude     
}