public class Second {
    int x;
    private String nome;

    //costrutttore
    public Second(int y) {
      x = y;
    }

    public String getNome(){
        return (nome + "!");
    }
    public void setNome(String nuovoNome){
        this.nome = nuovoNome;
    }

    public String Controllo(){
        return "Classe Madre";
    }


    class Classeinterna{
        public void stampa(){
           System.out.println("Classe interna");
        }

        //classe inner può accedere a metodi e campi della outer
        public int Stampo_x(){
            return x;
        }
    }

    static class Classeinterna_statica{
        public void stampa(){
            System.out.println("Classe interna statica");
        }

        //non posso fare lo stesso perchè essendo statica non può accedere a campi e metodi della outer
        /*
        public int Stampo_x(){
            return x;
        }
         */
    }
}


//QUESTE SOTTO SONO PER ESEMPIO, NON POSSO IMPLEMENTARLE PERCHÈ SONO FUORI DA SECOND E NON SONO
//PUBBLICHE


//Classi astratte servono solo da padri ma non possono creare oggetti
abstract class Animale{
    
    //metodi astratti NON posso avere un corpo
    public abstract void suono();
    
    public void stampa(){
        System.out.println("Classe astratta animale");
    }
  }
  
class Cane extends Animale{
  
    //metodo che era astratto nella classe madre DEVE essere implementato nella figlia 
    public void suono(){
        System.out.println("Bau Bau!");
    }
}

//posso anche utilizzare un interfaccia, si comporta similmente
//utile perchè posso implementare più interfacce con una virgola
interface Animale_interfaccia{
    public void suono(); //non posso definire metodi, sono automaticamente astratti e pubblici
    //gli attributi sono statici, pubblici e final
}

class Cane_daInterfaccia implements Animale_interfaccia{
    public void suono(){
        System.out.println("Bau Bau!");
    }
}
