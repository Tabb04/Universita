/*
//una stringa è un array di caratteri
//strtok suddivide una stringa in token più piccoli
//prende 2 parametri, la stringa e cosa uso per suddividere (delimitatore)

//esempio

s = uguale a "12,13,14,15"

tok = strtok (s, ",")
printf("%s\n", tok) --> da 12

//ciclo per continuare

while((tok = strtok(NULL, ","))!=NULL){
printf("Token:%s\n", tok) --> Token:12, Token:13, ..

//se ora stampo s di nuovo è rimasto -->12
}

//nella funzione nel while utilizzo NULL nel parametro della funzione per specificare di continuare sulla stringa che avevo già assegnato

//essenzialmente sostituisce alla virgola un \0 e poi restitusce il puntatore

1 2 , 1 3 , 1 4 , 1 5 , \0

al primo ciclo sostituisce la prima virgola con \0

1 2 \0 1 3 , 1 4 , 1 5 , \0

e restituisce 12

ora s punta al primo uno (se la stampo da 12)
tok ora cerca un altra virgola, la sostituisce e mette il puntatore a dove prima aveva messo \0

1 2 \0 1 3 \0 ...

tok stampa 13 perchè punta a il secondo 1 e si ferma a \0

ultimo passo tok punta all'ultimo \0 quindi restituisce NULL

enum si usa come tipo per gli switch per rendere più leggibile
*/


