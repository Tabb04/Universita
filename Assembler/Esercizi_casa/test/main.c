#include <stdio.h>
	
extern int Fucktorial(int n);

int main(){
	int numero = 5;
	int risultato;

	risultato = Fucktorial(numero);

	printf("Il fattoriale di %d è %d\n", numero, risultato);

	return 0;
}
