public class ContoLimitato implements BankAccount {
    private double saldo;
    private int mov;    //movimenti rimasti
    public ContoLimitato(double saldoIniziale, int movimenti){
        saldo = saldoIniziale;
        mov = movimenti;
    }


    public void versa(double somma){
        if (mov > 0){
            mov--;
            saldo += somma;
        }
    }

    public double getSaldo(){
        return saldo;
    }

    public boolean preleva(double somma){
        if(saldo>=somma && mov > 0){
            mov --;
            saldo -= somma;
            return true;
        }else{
            return false;
        }
    }
}
