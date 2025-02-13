

/*
LETTURA FILE, fopen, fclose
fgets, freeopen, fgetc, fprintf, fscanf, putchar







//per verificare l'apertura

FILE* fin = fopen("in.txt", "r");
if (fin){ //qui posso scrivere fin != NULL credo
    printf("File aperto\n");
    fclose(fin);
}

*/

#include<stdio.h>
#include<stdio.h>

int main() {
    FILE *file;  //mi serve un puntatore di tipo file
    char buffer[256];  // buffer per ogni riga letta


    file = fopen("test.txt", "r"); //modalità r lettura
    if (file == NULL) {  // Controllo errore
        perror("Errore nell'apertura del file");
        return 1;
    }

    printf("Contenuto del file:\n");


    //il primo dove mette, secondo è dimensione massima della riga, terzo da dove prende
    // Lettura e stampa delle righe
    while (fgets(buffer, sizeof(buffer), file)) { 
        printf("%s", buffer);  
    }

    /*
    se invece voglio mettere tutto il testo in una stringa:
    
    #define BUFF 1000
    int len = 0
    char testo[BUFF]
    while(!feof(file) && len<BUFF-1 && fgets(&testo[len], BUFF-len, file)){
        len = strlen(testo)
    }
    */



    if (freopen("test.txt", "w", file) == NULL) { //passo in modalita scrittura w
        perror("Errore nel riaprire il file in modalità scrittura");
        return 1;
    }

    printf("\nOra sovrascrivo il contenuto di 'testo.txt'.\n");

    // Scrittura di nuove righe nel file
    fprintf(file, "Questa è una nuova prima frase.\n");
    fprintf(file, "E questa è una nuova seconda frase.\n");

    // Chiusura del file
    if (fclose(file) != 0) {
        perror("Errore nella chiusura del file");
        return 1;
    }

    printf("Il nuovo contenuto è stato scritto correttamente in 'testo.txt'.\n");
    return 0;
}




/*
    FGETC FA COME FGETS MA PER CARATTERE A VOLTA    


    printf("Contenuto del file carattere per carattere:\n");

    // Lettura carattere per carattere
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);  // Stampa il carattere letto
    }
*/



/*
    FSCANF USO PER LETTURA DETTAGLIATA

    se ad esempio nel mio file ho
    Mario Rossi 25
    Luigi Verdi 30
    Anna Bianchi 22



    printf("Contenuto del file:\n");

    // Lettura formattata
    while (fscanf(file, "%s %s %d", nome, cognome, &eta) == 3) {
        printf("Nome: %s, Cognome: %s, Età: %d\n", nome, cognome, eta);
    }

    fclose(file);  // Chiusura del file
    return 0;
}


*/

/*
    PER GLI ERRORI
    devo usare include <errno.h>
    e controllare lo stato dello stream con 

    int feof(stream) //per vedere se sono arrivato a EOF

    inf ferror(stream) //non 0 se c'è stato errore

    int perror(const char* str)//errore codificato n errno, preceduto da str
*/

