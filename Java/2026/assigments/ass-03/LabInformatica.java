//Leggi testo in ass-03

import java.util.*;
import java.util.concurrent.*;;

    enum Categoria{
    PROFESSORE,
    TESISTA,
    STUDENTE
}

public class LabInformatica {

    public final int numComputer = 20;
    public final int idComputerTesisti = 19;    //ID del computer richiesto dai tesisti

    private List<Thread> threads;    //Lista dove memorizzo i riferimenti ai thread;

    private boolean[] computers;    //Array per tenere traccia dei computer assegnati (1-occupato, 0-libero)

    private int profWaiting = 0;
    private int tesiWaiting = 0;


    public LabInformatica(){
        threads = new ArrayList<>();
        computers = new boolean[numComputer];
    }


    public void start(int numProf, int numTesisti, int numStudenti){
        System.out.println("Laboratoio aperto.");

        for(int i = 0; i<numProf; i++){    //Creo gli utenti, un thread per ognuno di essi
            threads.add(new Thread(new User(Categoria.PROFESSORE, i, this)));
        }
        
        for(int i = 0; i<numTesisti; i++){
            threads.add(new Thread(new User(Categoria.TESISTA, i, this)));
        }

        for(int i = 0; i<numStudenti; i++){
            threads.add(new Thread(new User(Categoria.STUDENTE, i, this)));
        }

        //Eseguo uno shuffle della lista dei thread prima di avviarli

        Collections.shuffle(threads, new Random(System.currentTimeMillis()));

        for(Thread t: threads){
            t.start();
        }

        //attendo la terminazione di tutti i thread usando la join
        for(Thread t: threads){
            try{
                t.join();
            }catch(InterruptedException e){
                System.err.println("Interruzione durante l'attesa dei thread");
            }
        }
        System.out.println("Laboratorio chiuso");
    }

    public synchronized List<Integer> entrata(User u) throws InterruptedException{
        List<Integer> assegnati = new ArrayList<>();

        System.out.printf("%s con id=%d in attesa di entrare.\n", u.categoria.name(), u.id);

        //Procedo in maniera diversa a seconda del tipo utente

        switch(u.categoria){

            //I professori attendono finchè tutti i computer non sono disponibili e quindi occupano tutto il laboratorio

            case PROFESSORE:
                profWaiting++;  //Non ci sono problemi in teoria visto che questo metodo è synchronized

                while (!libero()) {
                    wait();
                }

                profWaiting--;
                for(int i = 0; i<computers.length; i++){
                    computers[i] = true;
                    assegnati.add(i);
                }
                break;
            

            case TESISTA:
                tesiWaiting++;

                while (profWaiting > 0 || computers[idComputerTesisti]) {
                    wait();
                }

                tesiWaiting--;
                computers[idComputerTesisti] = true;
                assegnati.add(idComputerTesisti);
                break;


            case STUDENTE:
                int id_pc = primoComputerLibero(); //Scorre l'array dei computer e mi da l'id del primo libero
                //se tutti occupati restituisce -1

                //Lo studente attende finchè ci sono professori che stanno aspettando, o se 
                //non ci sono computer disponibili, oppure se il computer assegnato è quello
                //dei tesisti e ci sono già tesisti prenotati per l'entrata

                while (profWaiting > 0 || id_pc == -1 || (tesiWaiting > 0 && id_pc == idComputerTesisti)) {
                    wait();
                    id_pc = primoComputerLibero();
                }

                computers[id_pc] = true;
                assegnati.add(id_pc);
                break;


            default:
                break;
        }

        System.out.printf("%s con id=%d entrato.\n", u.categoria.name(), u.id);
        return assegnati;
    }


    public synchronized void uscita(User u, List<Integer> occupati){

        //Libero tutti i computer che avevo occupato
        for(Integer id: occupati){
            computers[id] = false;
        }

        //Risveglio tutti gli utenti in attesa.
        //Al risveglio ognuno controllerà la propria condizione di attesa
        notifyAll();
        System.out.printf("%s con id=%d uscito.\n", u.categoria.name(), u.id);
    }


    private boolean libero(){   //true solo se sono tutti liberi
        for(int i = 0; i<computers.length; i++){
            if(computers[i]){
                return false;
            }
        }
        return true;
    }


    private int primoComputerLibero(){  //indice del primo pc libero.
        for(int i = 0; i<computers.length; i++){
            if(!computers[i]){
                return i;
            }
        }
        return -1;
    }


    public static void main(String[] args){

        //Verifica dei parametri
        if(args.length < 3){
            System.err.println("Esegui come: Laboratorio " + "<numProf> <numTesisti> <numStudenti>");
            System.exit(1);
        }

        int numProf = Integer.parseInt(args[0]);
        int numTestisti = Integer.parseInt(args[1]);
        int numStudenti = Integer.parseInt(args[2]);


        //Creo il laboratorio e faccio entrare gli studenti
        LabInformatica lab = new LabInformatica();

        lab.start(numProf, numTestisti, numStudenti);

    }

}


/** 
*Questa classe rappresenta il generico Utente del laboratorio
*/
class User implements Runnable{

    public Categoria categoria;

    public int id;
    public int numAccessi;
    public long workDelay;          //tempo in cui l'utente utilizza il laboratorio
    public long breakDelay;         //tempo tra un accesso e l'altro
    public int maxAccessi = 5;
    public long maxWork = 5000;     //Massimo tempo di lavoro
    public long maxBreak = 2000;    //Massimo tempo di pausa

    private LabInformatica lab;      //Riferimento al laboratorio

    public User(Categoria categoria, int id, LabInformatica lab){
        this.categoria = categoria;
        this.id = id;
        this.lab = lab;

        numAccessi = ThreadLocalRandom.current().nextInt(1, maxAccessi+1);
        workDelay = ThreadLocalRandom.current().nextLong(maxWork+1);
        breakDelay = ThreadLocalRandom.current().nextLong(maxBreak+1);
    }

    public void run(){
        try{
            for(int i = 0; i<numAccessi; i++){

                List<Integer> assegnati = lab.entrata(this);
                //Mi restituisce una lista di tutti i computer che mi sono stati assegnati
                //Se passo "this" passo il riferimento all'istanza corrente dell'oggetto in cui sono

                Thread.sleep(workDelay);
                lab.uscita(this, assegnati);
                //Quando esco restituisco i computer che avevo preso

                Thread.sleep(breakDelay);
            }

        }catch(InterruptedException e){
            System.out.printf("%s con id=%d ha terminato il lavoro.\n", categoria.name(), id);
        }
    }



}