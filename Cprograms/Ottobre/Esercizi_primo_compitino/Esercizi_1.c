
/*ESERCISI ALTRI SONO SU 22 OTTOBRE 2024*/


#include <stdio.h>
#include <stdlib.h>

int Sum_submatrix(int**, int);

int main(){
    int x, y, i, j;
    printf("%s", "Inserisci dimensione prima matrice quadrata: ");
    scanf("%d", &x);
    printf("%s", "Inserisci dimensione seconda matrice: ")
    scanf("%d", &y);

    
    int** A = (int**)malloc(sizeof(int*) * x);
    for(int i = 0; i<x; i++){
        A[i] = (int*)malloc(sizeof(int) * x);
    }

    for(int i = 0; i<x; i++){
        for(j = 0; j<x; j++){
            printf("\n%s", "Inserisci valore: ");
            scanf("%d",  &A[i][j]);
        }
    }

    int ris = Sum_submatrix(A, x);
    printf("%s", "La somma è: ");
    printf("%d", ris);
    return 0;
}

int Sum_submatrix(int** matrix, int x){
    int sum = 0;
    int i, j;
    for(int i = 0; i<x; i++){
        for(int j = 0; j<x; j++){
            sum += matrix[i][j];
        }
    }
    return sum;
}

