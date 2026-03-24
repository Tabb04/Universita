import java.util.Scanner;
import java.time.LocalDate;
import java.util.ArrayList;


public class Main{
  public static void main(String[] args){
    String nomeScelto;
    Second miaClasse = new Third_1(5); //costruttore

    Scanner scansione = new Scanner(System.in);
    System.out.println("Inserisci username: ");
    nomeScelto = scansione.nextLine();
    
    miaClasse.setNome(nomeScelto);
    System.out.println(miaClasse.getNome()); //essendo privati devo usare un setter e getter
    System.out.println(miaClasse.Controllo());

    //posso avere una classe interna sia statica che non

    //creo istanza di classe interna a partire da quella interna
    Second.Classeinterna miaClasseinterna = miaClasse.new Classeinterna();
    miaClasseinterna.stampa();

    //creo istanza di classe interna da sola perchè statica
    Second.Classeinterna_statica miaClasseinterna_statica = new Second.Classeinterna_statica();
    miaClasseinterna_statica.stampa();

    //enum
    Valori mioValore = Valori.BASSO;
    System.out.println(mioValore);

    //posso anche mostrare tutti i valori dell'enum con un for
    for(Valori valore : Valori.values()){
      System.out.println(valore);
    }

    //uso il pacchetto improtato time.Localdate, posso usare LocaTime per più precisione o LocalDateTime
    LocalDate miaData = LocalDate.now();
    System.out.println(miaData);
  
    //posso creare array dinamici con ArrayList
    ArrayList<String> nomi = new ArrayList<String>();
    nomi.add(nomeScelto);
    System.out.println("Inserisci un altro nome");
    String secondoNome = scansione.nextLine();
    nomi.add(secondoNome);
    System.out.println(nomi);

  
  }
    


}

//ci si accede come ad una classe
enum Valori{
  BASSO,
  MEDIO,
  ALTO
}







/*
public class Main{
  static void MioMetodo(String nome, int eta){
    System.out.println("Il nome è " + nome + ", età " + eta);
  }
  public static void main (String[] args){
    MioMetodo("Mirco", 12);
    System.out.println("Età sommatoria: " + Sommatoria(12));
    ControlloEta(12);
    System.out.println("Mancano " + Rimanente(12) + " anni.");
    
    //posso avere due metodi con lo stesso nome ma con argomenti diversi
    System.out.println(Somma(1, 2));
    System.out.println(Somma(1.5, 2.5));

  }

  static void ControlloEta(int num){
    System.out.println((num>17) ? "Maggiorenne": "Minorenne" );
  }
  
  static int Rimanente(int num){
    return 18-num ;
  }

  static int Somma(int x, int y){
    return x + y;
  }
  
  static double Somma(double x, double y){
    return x + y;
  }

  static int Sommatoria(int x){
    if (x<=0){
      return 0;
    }else{
      return x + Sommatoria(x-1);
    }
  }
}
*/



// CASTING VERSO PIÙ GRANDE
// byte -> short -> char -> int -> long -> float -> double

/*
public class Main {
  public static void main(String[] args) {
    int myInt = 9;
    double myDouble = myInt; 

    System.out.println(myInt);      // Outputs 9
    System.out.println(myDouble);   // Outputs 9.0
  }
}
*/


/*
//CASTING VERSO PIÙ PICCOLO
double -> float -> long -> int -> char -> short -> byte

public class Main {
  public static void main(String[] args) {
    double myDouble = 9.78d;
    int myInt = (int) myDouble; 

    System.out.println(myDouble);   // Outputs 9.78
    System.out.println(myInt);      // Outputs 9
  }
}
 */
