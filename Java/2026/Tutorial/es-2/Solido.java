public abstract class Solido {
    
    //Variabile d'istanza
    private double pesoSpecifico;


    //Costruttore
    public Solido (double ps){
        pesoSpecifico = ps;
    }

    //Metodo implementato
    public double peso(){
        return volume() * pesoSpecifico;
    }


    //Metodi astratti
    public abstract double volume();
    public abstract double superficie();

}
