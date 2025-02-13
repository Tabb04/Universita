#include <stdio.h>

int main (void){

    int a = 5;
    float b=(float)a/2; //se non includo float risulta 2.000000 invece che 2.500000
    printf("a=%d\nb=%f\n", a, b);
   return 0;
}


/*
int main(void){
    int a = 5;
    float b=a/2;
    printf("a=%d\nb=%f\n");
    return 0;
}

questo stampa a=5, b=2.0000000

*/

