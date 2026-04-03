public class Sfera extends Solido {
    
    //Variabili d'istanza
    private double raggio;


    //Costruttore
    public Sfera(double raggio, double ps){
        super(ps);
        this.raggio = raggio;
    }

    //Implementazione dei metodi astratti di Solido
    public double volume(){
        return 4/3 * Math.PI * Math.pow(raggio, raggio);
    }

    public double superficie(){
        return 4 * Math.PI * raggio * raggio;
    }

    //peso è definito nella superclasse


}
