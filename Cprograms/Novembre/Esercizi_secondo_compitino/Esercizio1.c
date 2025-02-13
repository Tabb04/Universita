//leggere da tastiera 10 interi e scriverli in un file . Provare sia con file testuale che con
//file binario(nel file testuale separarli con un carattere virgola). Infine stamparli separati da
//una virgola

#include<stdio.h>

int main(){
    FILE* file;
    int numeri[10];

    printf("Inserisci i 10 numeri separati da spazi:\n");
    for(int i = 0; i<10; i++){
        scanf("%d", &numeri[i]);
    }


    file = fopen("numeri.txt", "w");
    
    for(int i = 0; i<10; i++){
        fprintf(file, "%d", numeri[i]);
        fprintf(file, ",");
    }

    fclose(file);
    printf("Numeri inseriti su file testuale.\n");

    return 0;
}

//se voglio sostutire un numero nel mio file utilizzo prima fseek per spostare il puntatore
//poi utilizzo fwrite

//esempio scrivo nella terza posizione
/*
fseek(file, 2 * sizeof(int), SEEK_SET); seek set per partire in cima
fwrite(numero_da_inserire, sizeof(int), 1, file); //1 perchè scrivo solo una cosa
//potrei fare la fwrite =! 1 il controllo per vedere se è andato bene
//perchè la fwrite restituisce quanto scrive
*/