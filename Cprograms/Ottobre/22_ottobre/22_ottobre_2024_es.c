//array mono e multi, allocazione dinamica di memoria e manipolazione di stringhe

/*
//dato un array di interi, scrivere una funzione somma_con_trasformazione che calcoli la somma 
degli elementi di questo array. Questa funzione prende un puntatore a una funzione come parametro,
ed ha il seguente prototipo, data la seguente definizione di tipo custom

typedef int(*transforma_fn)(int); //tipo che è un puntatore di funzione che prende int e restituisce int

e il seguente prototipo per la funzione

int somma_con_trasformazione (int* a, int dim, transforma_fn trasforma)

la funzione passata come parametro determina come ogni elemento dell'array dovrebbe essere rasformato prima
di essere sommato. Le due possibili trasforamzioni sono:

1) valore_assoluto: calcola il valore assoluto dell'intero passato come parametro

2) quadrato: calcola il quadrato dell'intero passato come parametro

Il primo valore fornito da tastiera è dim array, Utilizzare l'allocazione dinamica della memoria.
Nello stampare il risultato, utilizzare la prima funzione con il valore assoluto, seguita 
da uno spazio e poi dalla funzione che calcola il quadrato


chiedere dim array e singoli valori, chiamo due volte somma con trasf. prima fa valore assoluto
poi quadrato

*/


/*
#include<stdio.h>
#include<stdlib.h>

typedef int(*transforma_fn)(int);

int valore_assoluto(int a){
    if (a<0) return -a;
    else return a;
}

int quadrato(int a){
    return a*a;
}

int somma_con_trasformazione (int* a, int dim, transforma_fn trasforma){
    int ris = 0;
    int i;
    for(i = 0; i<dim; i++){
        ris = ris + trasforma(a[i]);
    }
    return ris;
}


int main(){
    int dim1;
    int i;
    printf("%s", "Inserisci dimensione array: ");
    scanf("%d", &dim1);
    int* arr = (int*)malloc(dim1 * sizeof(int));


    for(i = 0; i<dim1; i++){
        printf("%s", "Inserisci Valore: ");
        scanf("%d", &arr[i]);
    }
    


    int ris1 = somma_con_trasformazione(arr, dim1, valore_assoluto);
    printf("%s", "Il vettore con valore assoluto è: ");
    printf("%d\n", ris1);

    int ris2 = somma_con_trasformazione(arr, dim1, quadrato);
    printf("%s", "Il vettore con quadrato è: ");
    printf("%d\n", ris2);

    return 0;
}

*/


/*

SOLUZIONE GIUSTA.
#include<stdlib.h>
include<stdio.h>

typedef int(*transforma_fn)(int);


int vaore_assoluto(int a){
    if (a<0) return -a
    else return a
}

int quadrato (int a){
    return a*a;
}

int somma_con_trasformazione(int* arr, int dim, trasforma_fn trasforma){
    int somma = 0;
    for(int i = 0; i<dim; i++){
        somma +- trasforma(arr[i]);
    }
    return somma;
}

int main(){
    int dim;

    scanf("%d", &dim);

    if (dim == 0){
        return 0;
    }

    int* A = (int*)malloc(dim * sizeof(int)); //alloco spazio per l'array

    for(int i = 0; i<dim; i++){
        scanf("%d\n", &A[i]);
    }

    int total = somma_con_trasformazione(A, dim, valore assoluto);
    prinf("%d", total);

    total = somma_con_trasformazione(A, dim, quadrato);
    prinf("%d\n", total):

    return 0;

}
*/





/*
Scrivere un programma che lavori su una lista concatenata, costituita da elementi interi,
il cui inserimento termina appena viene digitato un numero negativo.
Le operazioni da implementare devono essere contenuto in apposite funzioni e essere le seguenti,
dato un puntatire alla testa della lista denominato L:

1) print(L): stampa tutti glie elementi della lista L, uno per riga
2) insert(x, L): Inserrel'elemento in coda alla lista (NON ricorsiva)
3) insertRic(int x, L): come la predente MA ricorsiva
4) delete (L): elimina dalla lista l'ultimo elemento inserito, NON ricorsiva

Nella funzione main, testare le funzioni implementate, inserendo un elemento della lista per riga.
Usare la funzione ricorsiva se l'elemento è pari, non ricorsiva se è dispari. Stampare in Output
gli elementi della lista, uno per riga. Dopo aver inserito l'elemento negativo, svuotare la lista 
e stamparla nuovamento
*/

#include<stdio.h>
#include<stdlib.h>

typedef struct Nodo1{ //struct definisce un tipo, typedef cambia nome in Nodo subito dopo
    int val;
    struct Nodo1* next;
}Nodo;

typedef Nodo* Lista;

void InsertRic(int, Lista*); //inserisce in fondo alla lista

void Print(Lista); //stampa ricosrsivamente, partendo dalla testa

void Delete(Lista*);

void Insert(int , Lista*);

int main(){
    Lista l = NULL;
    int x ;

    do{
        printf("\n%s", "Inserisci valore lista: ");
        scanf("%d", &x);
        InsertRic(x, &l); 
    }while (x>0);

    printf("%s", "La lista è: ");
    Print(l);
    Delete(&l);
    printf("\n%s", "La lista cancellata è: ");
    Print(l);
    InsertRic(200, &l);
    InsertRic(500, &l);
    InsertRic(600, &l);

    Print(l);

    return 0;
}

void InsertRic(int x1, Lista* l1){ //devo passare il puntatore per modificare
        if ((*l1) == NULL) {
            //caso lista vuota   
            Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo)); //nuovo è un indirizzo di memoria
            
            if (nuovo == NULL){ //se da errore in allocare memoria
                exit(1);
            }

            nuovo->val = x1;
            // nuovo->next = *l1; non ho idea a che cazzo servisse ma funziona comunque

            *l1 = nuovo; //nuovo è del tipo Lista
        }else {
            InsertRic(x1, &((*l1)->next)); 
        }
}

void Print(Lista l1){
    if (l1 == NULL) return;
    printf("%d ", l1->val);
    Print(l1->next);
}

void Delete(Lista* l1){
    if (*l1 == NULL){
        return;
    }

    Nodo* Actual = *l1;
    Nodo* Prec = NULL;

    while(Actual->next != NULL){
        Prec = Actual;
        Actual = Actual->next;
    }

    free(Actual);

    if(Prec == NULL){
        *l1 = NULL;
    }else{
        Prec->next = NULL;
    }
}

void Insert(int x, Lista* l1){
    if (*l1 == NULL){
        Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo));
        if(nuovo == NULL){
            exit(1);
        }
        nuovo->val = x;
        nuovo->next = NULL;
        *l1 = nuovo;
    }

    Nodo* actual = *l1;

    while(actual->next != NULL){
        actual = actual->next;
    }

    Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo));
    nuovo->val = x;
    nuovo->next = NULL;
    actual->next = nuovo; //? non so se usare actual o *l1?   
}




/*


SOLUZIONE, molti uguali probabilmente sono -



typedef struct n{
    int val;
    struct n*next;
} Nodo;

typedef Nodo* Lista;


void prinf(Lista );
void insert(int, Lista*)



int main(){

    Lista l = NULL;
    int x, num_el=0;

    do{
        scanf("%d", &x);
        if (x%2==0)
            insertRic(x, &l);
        else   
            insert(x, &l);
    }while(x>=0);

    print(l);
    scanf("%d", x);
    for(ont i=0; i...){
        delete (x, &l); //ciclo che cancello elementi in teoria
        print(l) ??????
    }

    //inserisco, ricorsivamente, un elemento in fondo alla lista

    void insertRic(int x, Lista* l){

        if ((*l)==NULL){
            Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo));

            if (nuovo==NULL)
                exit(l);

            nuovo->val=x;
            nuovo->next=*l;

            *l=nuovo;
        }
        else
            insertRic(x, &((*l)->next));
    }

//stampa tutti gli elementi di una lista, partendo da testa, ricorsiva

void print(Lista l){
    if (l==NULL)
        return;
    printf("%d\n", l->val);
    prinf(l->next);
}

void delete (int x, Lista* L){
    if (*L==NULL)
        return;
    Nodo *attuale = *L;
    Nodo *prec = NULL;

    while(attuale->next!=NULL){
        prec=attuale;
        attuale=attuale->next;
    }

    free(attuale);
    if (prec==*L)
        *L=NULL;
    else
        prec ->next=NULL
}


//inserisco un elemento in fondo alla lista (non ricorsiva)

void insert(int x, Lista* l){

    Nodo* prec, *succ, *nuovo; //per succ e nuovo devo mettere asterisco davanti altrimenti non capisce che è puntatore a nodo, C è stupido

    nuovo=(Nodo*)malloc(sizeof(Nodo));

    if(nuovo==NULL) //se malloc non riesce ad allocare memoria ritorna un NULL
        exit(l);

    nuovo->val=x;
    nuovo->next = NULL;

    prec = NULL;
    succ=*l
    
    while(succ!=NULL){
        succ=succ->next;
        //HA MODIFICATO MA NON HO POTUTO LEGGERE DIO STRONZO
    }
    nuovo->next = succ;
    if (prec!=NULL)
        prec->next = nuovo;
    else
        *l=nuovo;
}

}
*/




/*
Utilizzando l'implementazione vista a lezione delle liste concatenate, si deve implementare
una struttura dati rappresentante una pila(struttura LIFO) e una serie di funzioni che permettono
la seguente operazioni:

1) push inserisce un elemento nella pila
2) pop rimuove il primo elemento della pila e lo ritorna. Torna 0 se è vuota.
3) length calcola la lunghezza della pila in modo ricorsivo
4) print stampa gli elementi della pila, iniziando con l'elemento in testa in modo ricorsivo

Scrivere una funzione Main che legge da standard input una serie di numeri interi e, utilizzando
le funzioni definite sopra esegue le seguenti opeazioni.

1) Se il numero è diverso da 0 ed è un multiplo di 3, viene inserito nella pila il numero diviso 3
2)se il numero è diverso da 0 e non è mult 2, viene inserito nella pila
3)se il numero è 0, viene eliminato dalla pila, solo se l'elemento da eliminare è dispari o 
la prila attualmente contiene più di 3 elementi. (parla del primo elemento)

//riuscire a tenere un contatore della dimensione.

4)il programma si ferma quando si leggono 3 0 di fila

Alla fine della funzione main defve essere stampato in stdout il contenuto della pila e la sua
lunghezza, con tutti gli elemetni della pila su un'unica riga, separati da uno spazi e la lunghezza
della pila come ultimo valore


*/


/*

soluzioni sono nelle foto/slide*/


/*
//concatena due stringhe chieste in output (e lunghezza) in un nuovo spazio

#include <stdio.h>
#include <stdlib.h>
#define MAXLEN 1001


char* my_strcat1(char* , char*, int, int);

int main(){
    
    int x, y;
    

    printf("%s", "Inserisci prima dimensione: ");
    scanf("%d", &x);
    char* a1 = (char*)malloc(sizeof(char) * x);

    printf("\n%s", "Inserisci Prima stringa: ");
    scanf("%s", a1);
    
    printf("\n%s", "Inserisci prima dimensione: ");
    scanf("%d", &y);
    char* a2 = (char*)malloc(sizeof(char) * y);

    printf("\n%s", "Inserisci Seconda stringa: ");
    scanf("%s", a2);



    char* a3 = my_strcat1(a1, a2, x, y);
    printf("%s", a3);
    return 0;
}

char* my_strcat1(char* x, char* y, int len1, int len2){
    char* xy = (char*)malloc(sizeof(char)*(len1+len2+1));
    
    int j=0, i;


    for(i = 0; i<len1; i++){
         xy[j] = x[i];
         j++;
    }

    for(i = 0; i<len2; i++){
         xy[j] = y[i];
         j++;
    }
    xy[j] = '\0';
    return xy;
}

*/


/*
//implementazione di una Pila, simile a Lista

#include <stdio.h>
#include <stdlib.h>

typedef struct ll_node_struct{
    int info;
    struct ll_node_struct* next;
} Nodo;
typedef Nodo* Pila;

void Push(int , Pila*);

int Pop(Pila*);

int Length(Pila*, int);

void Print(Pila);

int main(){
    Pila pila = NULL;
    Push(10, &pila);
    Push(20, &pila);
    Push(30, &pila);
    Push(40, &pila);
    Push(50, &pila);

    int lun = Length(&pila, 0);
    printf("\n%s", "La lunghezza è: ");
    printf("%d\n", lun);
    Pop(&pila);
    // Stampa per verificare il contenuto
    Print(pila);


    return 0;

}

void Push(int x, Pila* p){
    Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo));
    if(nuovo == NULL){
        exit(1);
    }
    nuovo->info = x;
    nuovo->next = *p;
    *p=nuovo;
}



int Pop(Pila* p){
    if(*p == NULL){
        return 0;
    }
    Nodo* temp = *p;
    Nodo* succ = temp->next;
    *p = succ;
    return temp->info;
}

int Length(Pila* p, int x){
    if (*p == NULL){
        return x;
    }else{
        return Length(&((*p)->next), x+1);
    }
}

void Print(Pila p){
    if (p == NULL){
        return;
    }
    printf("%d\n", p->info);
    Print(p->next);
}
*/

