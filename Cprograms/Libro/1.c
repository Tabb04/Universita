/*
#include <stdio.h>

int main(void){

int integer1, integer2;
int test1, test2;


printf("Inserisci primo numero: ");
scanf("%d", &integer1);


printf("\nInserisci secondo numero: ");
scanf("%d", &integer2);

printf("\nInserisci 2 valori ");
scanf("%d%d", &test1, &test2);
printf("\n%d", test1);
printf("\n%d", test2);

int sum = integer1 + integer2;

printf("\nLa somma è: %d", sum);
printf("\nLa somma è: %d", integer1 + integer2);
}

/*
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
int main (void){
    int height_inch, weight;
    float height_meters, bmi;
    int molt = 703;
    char choosen[10];
    char stringa[] = "imperiale";

    printf("Misurazione BMI, imperiale o metrico? ");
    scanf("%9s", choosen);
    for (int i = 0; choosen[i] != '\0'; i++) {
        choosen[i] = tolower(choosen[i]);
    }

    if (strcmp(choosen, "imperiale") == 0){
        printf("\nInserisci il tuo peso intero in libbre: ");
        scanf("%d", &weight);
        
        printf("\nInserisci la tua altezza intera in pollici: ");
        scanf("%d", &height_inch);
        
        bmi = (float)(weight * molt) / (height_inch * height_inch);
    
    }else{
        printf("\nInserisci il tuo peso intero il chili: ");
        scanf("%d", &weight);

        printf("Inserisci la tua altezza in metri: ");
        scanf("%f", &height_meters);

        bmi = (float)(weight)/(height_meters * height_meters);
    }
    puts("----------------------------------------");
    puts("Tabella dell'indice di massa corporea: ");
    puts("Sottopeso: meno di 18.5");
    puts("Normale: tra 18.5 e 24.9");
    puts("Sovrappeso: tra 25 e 29.9");
    puts("Obeso: 30 o oltre");

    printf("\nIl tuo BMI è: %.2f\n", bmi);
    if (bmi < 18.5) {
        printf("Sei sottopeso.\n");
    } else if (bmi >= 18.5 && bmi <= 24.9) {
        printf("Hai un peso normale.\n");
    } else if (bmi >= 25 && bmi <= 29.9) {
        printf("Sei in sovrappeso.\n");
    } else if (bmi >= 30) {
        printf("Sei obeso.\n");
    } else {
        printf("Errore nei calcoli.\n");
    }

    return 0;

}
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int square(int y);

int main (void){
    for (int x=1; x<=10; x++){
        printf("%d ", square(x));
    }

    puts("");
}

int square(int y){
    return y*y;
}

*/
/*
#include <stdio.h>
#include <stdlib.h>

int main (void){
    int var1, var2, sum;

    printf("Inserisci due numeri interi separati da un invio: ");
    scanf("%d%d", &var1, &var2);

    sum = var1 + var2;

    printf("La somma è: %d\n", sum);
    return 0;
}   

*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void){
    float raggio, area;

    printf("Inserisci il valore del raggio del cerchio: ");
    scanf("%f", &raggio);

    area = M_PI * raggio * raggio;

    printf("\nL'area è: %f", area);
    return 0;
}
*/

/*
#include <stdio.h>
#include <stdlib.h>
int main(void){
    int val;
    printf("Inserisci un numero intero: ");
    scanf("%d", &val);

    if (val%2==0){
        printf("%d è pari.\n", val);
    }else{
        printf("%d è dispari\n.", val);
    }
    return 0;
}
*/

/*
#include <stdio.h>
#include <stdlib.h>

int main(void){
    int num, i;
    printf("Inserisci un numero intero positivo: ");
    scanf("%d", &num);

    for(i = 0; i<num-1; i++){
        printf("%d, ", i*2);
    }
    printf("%d.\n", i*2);
    return 0;
}
*/


/*
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void){
    int num, temp;
    int max = -INFINITY;
    int min = INFINITY;

    printf("Inserisci quanti numeri vuoi analizzare: ");
    scanf("%d", &num);

    for(int i = 0; i<num; i++){
        printf("Inserisci valore intero: ");
        scanf("%d", &temp);
        if (temp < min){
            min = temp;
        }
        if (temp > max){
            //faccio if e non else perchè nel caso ci sia un solo elemento non funzionerebbe
            max = temp;
        }
    }

    printf("Il maggiore è %d, il minore è %d\n", max, min);
    return 0;
}
*/


/*
#include <stdio.h>
#include <stdlib.h>

int main(void){
    unsigned int num;

    printf("Inserisci un numero intero positivo: ");
    scanf("%d", &num);

    if (num < 2){
        printf("Il numero non è primo");
        return 0;
    }
    for(int i = 2; i<num/2; i++){
        if (num % i == 0){
            printf("Il numero non è primo");
            return 0;
        }
    }
    printf("Il numero è primo");

    return 0;

}
*/

/*
#include <stdio.h>
#include <stdlib.h>

int main (void){
    int m, n, temp, sum;
    int** matrice;
    
    printf("Inserisci altezza e larghezza della matrice: ");
    scanf("%d%d", &m, &n);

    matrice = (int**)malloc(m * sizeof(int*));
    if (matrice == NULL){
        return 1;
    }
    for(int i = 0; i<m; i++){
        matrice[i] = (int*)malloc(n * sizeof(int));
        if(matrice[i] == NULL){
            return 1;
        }
    }

    for(int i = 0; i<m; i++){
        for(int j = 0; j<n; j++){
            printf("Inserisci valore: ");
            scanf("%d", &temp);
            matrice[i][j] = temp; //non so se va bene passato così temp
            sum += temp;
        }
    }
    printf("La somma di tutti i valori vale: %d\n", sum);
 
    
    //test matrice visualizzazione
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrice[i][j]);
        }
        printf("\n");
    }
    
    for(int i = 0; i<m; i++){
        free(matrice[i]);
    }
    free(matrice);
    return 0;

}
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(void){
    int m, n, search;
    int** matrice;
    bool found = false;

    printf("Inserisci altezza e larghezza della matrice: ");
    scanf("%d%d", &m, &n);

    matrice = (int**)malloc(m * sizeof(int*));
    if (matrice == NULL){
        return 1;
    }

    for(int i = 0; i<m; i++){
        matrice[i] = (int*)malloc(n * sizeof(int));
        if(matrice[i] == NULL){
            return 1;
        }
    }

    for(int i = 0; i<m; i++){
        for(int j = 0; j<n; j++){
            printf("Inserisci valore [%d][%d]: ", i, j);
            scanf("%d", &matrice[i][j]);
        }
    }
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", matrice[i][j]);
        }
        printf("\n");
    }

    printf("Inserisci il valore da cercare: ");
    scanf("%d", &search);
    int i = 0;
    while(i<m && found == false){
        for(int j = 0; j<n; j++){
            if(matrice[i][j] == search){
                printf("Il valore %d è stato trovato in posizione [%d][%d]\n", search, i, j);
                found = true;
                break;
            }
        }
        i++;
    }
    if (found == false){
        printf("Il valore %d non è presente nella matrice.\n", search);
    }
    
    for(int i = 0; i<m; i++){
        free(matrice[i]);
    }
    free(matrice);
    return 0;

}
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define MAX 100

int main(void){
    char* studenti[5];
    char temp[MAX];
    bool found = false;

    for(int i = 0; i<5; i++){
        studenti[i] = (char*)malloc(sizeof(char) * MAX);
        if(studenti[i] == NULL){return 1;}

        printf("Inserisci nome dello studente n°%d: ", i+1);
        scanf("%99s", studenti[i]);
    }
    printf("\nInserisci nome da cercare: ");
    scanf("%99s", temp);

    for(int i = 0; i<5; i++){
        if (strcmp(studenti[i], temp) == 0){
            found = true;
            printf("%99s è stato trovato in posizione %d\n", temp, i+1);
            break;  
        }
    }
    if (!found) {printf("Il nome %s non è stato trovato\n", temp);}

    return 0;
}
*/

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    char nome[50];     
    int quantita;      
    float prezzo;      
} Prodotto;

void Inserisciprodotto(Prodotto* p){
    printf("Inserisci nome: ");
    scanf("%49s", p->nome);

    printf("Inserisci quantità: ");
    scanf("%d", &p->quantita);

    printf("Inserisci prezzo: ");
    scanf("%f", &p->prezzo);

}

void StampaProdotto(Prodotto* p){ //devo usare puntatore anche se stampa probabilmente CONTROLLA
    printf("Nome: %s, Quantità: %d, Prezzo: €%.2f", p->nome, p->quantita, p->prezzo);
}

int main(void){
    int num;
    Prodotto* inventario;
    
    //non utilizzo puntatore di puntatori perchè sto allocando un blocco continuo di memoria.
    //un doppio puntatore servirebbe se ogni prodotto fosse un puntatore a una struttura separata
    //(non in memoria contigua). Questo aumenta la complessità senza necessità.


    printf("Quanti prodotti vuoi inserire?: ");
    scanf("%d", &num);

    inventario = (Prodotto*)malloc(sizeof(Prodotto) * num);
    if (inventario == NULL){return 1;}
    
    for(int i = 0; i<num; i++){
        printf("\nInserisci i dettagli del prodotto n°%d\n", i+1);
        Inserisciprodotto(&inventario[i]);
    }

    puts("\nInventario:");
    for(int i = 0; i<num; i++){
        StampaProdotto(&inventario[i]);
        puts("");
    }

    return 0;
}
*/

//Gestione inventario con file di memoria

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NOME 50
#define FILE_INVENTARIO "inventario.txt"


typedef struct {
    char nome[MAX_NOME];
    int quantita;
    float prezzo;
} Prodotto;

//prototipi
int Leggifile(Prodotto**);
void Inserisciprodotto();
void Aggiornafile(Prodotto* prodotti, int count, bool sovrascrivi);
void Elencaprodotti();
void Eliminaprodotto();
void Calcolovalore();


int main(void){
    int choice;
    do{
        printf("\nGestione Inventario:\n");
        printf("1. Inserisci nuovo prodotto\n");
        printf("2. Elenca tutti i prodotti\n");
        printf("3. Elimina un prodotto\n");
        printf("4. Valore complessivo prodotto\n");
        printf("0. Esci\n");
        printf("Scegli un'opzione -> ");
        scanf("%d", &choice);
        getchar();

        switch(choice){
            
            case 1:
                Inserisciprodotto();
                break;
            case 2:
                Elencaprodotti();
                break;
            case 3:
                Eliminaprodotto();
                break;
            case 4:
                Calcolovalore();
                break;
            case 0:
                printf("Esco dal programma.\n");
                break;
            default:
                printf("Inserisci un valore compreso tra 0 e 3.\n");
        }

    }while(choice != 0);

    return 0;
}

void Inserisciprodotto(){
    Prodotto new;
    Prodotto* prodotti = NULL; //non so se è necessario inizializzarlo a NULL ma probabilmente si
    int count = Leggifile(&prodotti);
    bool found = false;

    printf("Inserisci Nome: ");
    scanf("%49s", new.nome);
    printf("Inserisci quantita: ");
    scanf("%d", &new.quantita);
    printf("Inserisci prezzo (se prodotto già presente rimarrà originale): ");
    scanf("%f", &new.prezzo);

    for(int i = 0; i<count; i++){
        if(strcmp(new.nome, prodotti[i].nome)==0){
            prodotti[i].quantita += new.quantita;
            found = true;
            break;
        }
    }
    if(!found){
        prodotti = realloc(prodotti, (count + 1) * sizeof(Prodotto));
        if (prodotti == NULL) {
            perror("Errore realloc");
            exit(1);
        }
        prodotti[count++] = new;
    }

    Aggiornafile(prodotti, count, found);

    free(prodotti);
    printf("Prodotto inserito correttamente!\n");
    return;
}

int Leggifile(Prodotto** prodotti){
    FILE* file = fopen(FILE_INVENTARIO, "r");
    if (file == NULL) {
        *prodotti = NULL; // Nessun prodotto presente
        return 0;
    }

    Prodotto temp;
    int count = 0;

    while(fscanf(file, "%49s %d %f", temp.nome, &temp.quantita, &temp.prezzo) == 3){
        *prodotti = realloc(*prodotti, (count+1) * sizeof(Prodotto));
        if (*prodotti == NULL) {
            perror("Errore realloc");
            exit(1);
        }
        (*prodotti)[count] = temp;
        count++; //guarda se fare [count++] fa differenza, in teoria no
    }
    
    fclose(file);
    return count;
}

void Aggiornafile(Prodotto* prodotti, int count, bool sovrascrivi){

    //se devo modificare la quantità di un elemento potrei ciclare finche non trovo il numero e aumentare
    //nel mio caso se devo aumentare una quantità riscrivo tutto
    FILE* file;
    if (sovrascrivi){ //se dovevo modificare solo quantità
        file = fopen(FILE_INVENTARIO, "w"); //riscrivo tutto
        if (file == NULL) {
            perror("Errore apertura file");
            exit(1);
        }

        for(int i = 0; i<count; i++){
            fprintf(file, "%s %d %.2f\n", prodotti[i].nome, prodotti[i].quantita, prodotti[i].prezzo);//non so pk uso "." invece d i "->""

        }
    }else{ //se aggiungo prodotto nuovo
        file = fopen(FILE_INVENTARIO, "a"); //a mette buffer in fondo al file
        fprintf(file, "%s %d %.2f\n", prodotti[count - 1].nome, prodotti[count - 1].quantita, prodotti[count - 1].prezzo);
    }

    fclose(file);
}

void Elencaprodotti(){
    Prodotto* prodotti = NULL;
    int count = Leggifile(&prodotti);

    
    if(count == 0){
        puts("\nNessun prodotto in inventario\n");
    }else{
        puts("\nElenco prodotti:");
        puts("--------------------");
        for(int i = 0; i<count; i++){

            printf("Nome: %s, Quantità: %d, Prezzo: %.2f\n", prodotti[i].nome, prodotti[i].quantita, prodotti[i].prezzo);//non so se necessario .2
        }
    }
    free(prodotti);
    

}

void Eliminaprodotto(){
    char nome[MAX_NOME];
    Prodotto* prodotti = NULL;
    int count = Leggifile(&prodotti);
    bool found = false;

    printf("Nome del prodotto da eliminare: ");
    scanf("%49s", nome);
    
    if (count == 0) {
        printf("Nessun prodotto presente nell'inventario.\n");
        return;
    }

    for(int i = 0; i<count; i++){
        if (strcmp(prodotti[i].nome, nome) == 0){
            found = true;
            for(int j = i; i<count - 1; i++){
                prodotti[j] = prodotti[j + 1];
            }
            count--;
            break;
        }
    }
    if(found){
        Aggiornafile(prodotti, count, true);
        printf("Prodotto Eliminato con successo!\n");
    }else{
        printf("Prodotto inserito non presente in inventario\n");
    }
    return;
}

void Calcolovalore(){
    char nome[MAX_NOME];
    Prodotto* prodotti = NULL;
    int count = Leggifile(&prodotti);
    float sum = 0;

    if (count == 0) {
        printf("Nessun prodotto presente nell'inventario.\n");
        return;
    }

    printf("Inserisci nome da calcolare: ");
    scanf("%49s", nome);

    for(int i = 0; i<count; i++){
        if(strcmp(prodotti[i].nome, nome) == 0){
            sum = (float)prodotti[i].prezzo * prodotti[i].quantita;
            break;
        }
    }
    
    if (sum != 0){
        printf("Il valore totale è: %.2f.\n", sum);
    }else{
        printf("Prodotto non presente\n");
    }
    return;

}
*/

    /*
    //funzione alternativa per la stampa
    FILE *file = fopen(FILE_INVENTARIO, "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return;
    }

    char nomeProdotto[100];
    int quantita;
    float prezzo;

    printf("Contenuto del file:\n");
    while (fscanf(file, "%s %d %f", nomeProdotto, &quantita, &prezzo) == 3) {
        printf("Prodotto: %s, Quantità: %d, Prezzo: %.2f\n", nomeProdotto, quantita, prezzo);
    }

    fclose(file);
    */


