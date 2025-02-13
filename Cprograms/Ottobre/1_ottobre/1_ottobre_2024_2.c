#include <stdio.h>
int a;

int main(void){
    int somma = 0, n, attuale;
    scanf("%d\n", &n);
    for(int i=0; i<n; i++){
        scanf("%d", &attuale);
        somma += attuale;
    }
    printf("%d\n", somma);
    return 0;
}


/*
se faccio float a = 0.565
e poi stampo con 
prinf("a: %d\n", a)

%d serve per stampare intero ma io ho un float quindi stampa robe random perchè i numeri
con virgola sono memorizzati con mantissa (es. 1.3454 * 10^-3)


se invece faccio
prinf("a: %d\n", (int)a) stampa solo 0, perchè me lo stampa convertendo in intero

per avere valore corretto devo usare %f

per invece stampare un carattere char devo usare %c
*/