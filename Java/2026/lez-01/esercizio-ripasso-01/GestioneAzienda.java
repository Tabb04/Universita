import java.util.*;

abstract class Persona{
    protected String nome;
    protected String indirizzo;
    protected String telefono;
    
    public Persona(String nome, String indirizzo, String telefono){
        this.nome = nome;
        this.indirizzo = indirizzo;
        this.telefono = telefono;
    }

    //Ogni persona saprà come stampare la propria paga
    public abstract void StampaPaga();

}



class Volontario extends Persona{
    public Volontario(String nome, String indirizzo, String telefono){
        super(nome, indirizzo, telefono);
    }

    //Sovrascrivo
    @Override
    public void StampaPaga(){
        System.out.println(nome + " (Volontario): Grazie!");
    }

}



abstract class Dipendente extends Persona{
    protected String codiceFiscale;
    protected double pagaBase;  //Mensile per impiegati, oraria per gli altri

    public Dipendente(String nome, String indirizzo, String telefono, String codiceFiscale, double pagaBase){
        super(nome, indirizzo, telefono);

        this.codiceFiscale = codiceFiscale;
        this.pagaBase = pagaBase;
    }



    public abstract double calcolaPaga();


    @Override
    public void StampaPaga(){
        System.out.printf("%s (CF: %s): Stipendio = €%.2f\n", nome, codiceFiscale, calcolaPaga());
    }
}


class ImpiegatoLivello1 extends Dipendente{
    public ImpiegatoLivello1(String nome, String indirizzo, String telefono, String codiceFiscale, double pagaBase){
        super(nome, indirizzo, telefono, codiceFiscale, pagaBase);
    }

    @Override
    public double calcolaPaga(){
        return pagaBase;
    }
}

class ImpiegatoLivello2 extends ImpiegatoLivello1{

    private double bonus;

    public ImpiegatoLivello2(String nome, String indirizzo, String telefono, String codiceFiscale, double pagaBase, double bonus){
        super(nome, indirizzo, telefono, codiceFiscale, pagaBase);
        
        this.bonus = bonus;
    }

    @Override
    public double calcolaPaga(){
        return super.calcolaPaga() + bonus;
    }

}


class Lavoratore_A_Ore extends Dipendente{
    private int oreLavorate;

    public Lavoratore_A_Ore(String nome, String indirizzo, String codiceFiscale, double pagaOraria, int oreLavorate){
        super(nome, indirizzo, indirizzo, codiceFiscale, pagaOraria);
        this.oreLavorate = oreLavorate;
    }

    @Override
    public double calcolaPaga(){
        return pagaBase * oreLavorate;
    }
}



public class GestioneAzienda{
    
    public static void main(String [] args){
        Scanner scanner = new Scanner(System.in);
        List<Persona> staff = new ArrayList<>();

        System.out.println("--- IMPOSTAZIONI INIZIALI AZIENDA ---");
        System.out.print("Inserisci lo stipendio base per Impiegati livello 1 e 2: $");
        double stipendioBase = scanner.nextDouble();
        
        System.out.print("Inserisci il bonus per gli Impiegati livello 2: $");
        double bonusLivello2 = scanner.nextDouble();


        System.out.print("Inserisci la paga oraria standard: $");
        double pagaOraria = scanner.nextDouble();


        staff.add(new ImpiegatoLivello1("Mario-impiegato1", "Via Roma 1", "2211214312", "MRRAFAOA", stipendioBase));
        staff.add(new ImpiegatoLivello2("Luigi-impiegato2", "Via paolo savi 2", "5311253151", "LUIGG823", stipendioBase, bonusLivello2));
        

        //Ipotizzo che il lavoratore abbia lavorato 120 ore questo mese
        staff.add(new Lavoratore_A_Ore("Giulia-aore", "Via napolo 1", "GLIU2322", pagaOraria, 120));

        staff.add(new Volontario("Anna-volontaria", "Via firenze 12", "1226657432"));



        System.out.println("Calcolo stipendi");


        for(Persona p: staff){
            p.StampaPaga();
        }

        scanner.close();

    }
}