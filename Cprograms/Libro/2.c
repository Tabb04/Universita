#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#define MAX_NOME 50

typedef struct Nodo {
    char nome[MAX_NOME];
    int posto;
    struct Nodo* next;
} Nodo;

//prototipi
void AggiungiPrenotazione(Nodo** lista);
void VisualizzaPrenotazioni(Nodo* lista);
void CancellaPrenotazione(Nodo** lista);
void LiberaMemoria(Nodo* lista);
void ScriviSuFile();

//funzione supporto
int is_numeric(char* str);

int main(void){

    int choice;
    Nodo* lista = NULL;
    Nodo** lista1 = &lista;

    do{
        printf("Sistema di prenotazione cinema\n");
        printf("1. Aggiungi prenotazione\n");
        printf("2. Visualizza prenotazioni\n");
        printf("3. Cancella prenotazione\n");
        printf("0. Esci\n");
        printf("Inserisci un opzione -> ");
        scanf("%d", &choice);

        switch (choice){
        case 1:
            AggiungiPrenotazione(lista1);
            break;
        case 2:
            VisualizzaPrenotazioni(lista);
            break;
        case 3:
            CancellaPrenotazione(lista1);
            break;
        case 0:
            LiberaMemoria(lista);
            puts("Uscita.");
            break;
        default:
            puts("Inserisci un valore tra 0 e 3.");
            break;
        }

    }while(choice != 0);

    return 0;

}

void AggiungiPrenotazione(Nodo** lista){
    Nodo* nuovo = (Nodo*)malloc(sizeof(Nodo));
    if (nuovo == NULL){exit(1);}

    printf("Inserisci nome dello spettatore: ");
    scanf("%49s", nuovo->nome);
    printf("Inserisci il numero del posto: ");
    scanf("%d", &nuovo->posto);

    if(nuovo->posto < 0 || nuovo->posto > 100){
        printf("I posti sono disponibili da 0-100\n");
        puts("------------------------------------------");
    
    }else{
        Nodo* current = *lista;
        while(current != NULL){
            if (current->posto == nuovo->posto){
                printf("Posto già occupato da %s.\n", current->nome);
                puts("------------------------------------------");
                free(nuovo);
                return;
            }
            if (strcmp(current->nome, nuovo->nome) == 0){
                printf("Hai già una prenotazione nel posto %d.\n", current->posto);
                puts("------------------------------------------");
                free(nuovo);
                return;
            }
            current = current->next;
        }
    //nuova prenotazione la metto in cima alla lista
    nuovo->next = *lista; //il next di nuovo è cio a cui puntava la lista prima
    *lista = nuovo; //lista ora punta a nuovo
    printf("Prenotazione aggiunta con successo!\n");
    puts("------------------------------------------");
    return;
    }
}

void VisualizzaPrenotazioni(Nodo* lista){
    puts("Lista delle prenotazioni con numero di posto:");

    Nodo* current = lista;
    while(current != NULL){
        printf("Nome: %s, Posto: %d\n", current->nome, current->posto);
        current = current->next;
    }
    puts("------------------------------------------");
    return;
}
void CancellaPrenotazione(Nodo** lista) {
    if (*lista == NULL) { // Controlla se la lista è vuota
        printf("La lista delle prenotazioni è vuota.\n");
        puts("------------------------------------------");
        return;
    }

    char input[MAX_NOME];
    int numero;

    printf("Inserisci il numero di posto da cancellare o il nome della prenotazione: ");
    getchar(); // Consuma eventuale newline lasciato da scanf precedente
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = '\0'; // Rimuove il newline dalla stringa

    Nodo* current = *lista;
    Nodo* precedente = NULL;

    if (is_numeric(input)) { // Input numerico (numero di posto)
        numero = atoi(input);
        if (numero < 0 || numero > 100) {
            puts("Inserire un numero di posto valido!");
            puts("------------------------------------------");
            return;
        }

        // Controlla il primo nodo
        if (current->posto == numero) {
            *lista = current->next; // Il primo nodo è da eliminare
            free(current);
            printf("Prenotazione per il posto %d eliminata.\n", numero);
            puts("------------------------------------------");
            return;
        }

        // Scorri la lista per trovare il nodo da eliminare
        while (current != NULL) {
            if (current->posto == numero) {
                precedente->next = current->next;
                free(current);
                printf("Prenotazione per il posto %d eliminata.\n", numero);
                puts("------------------------------------------");
                return;
            }
            precedente = current;
            current = current->next;
        }
    } else { // Input testuale (nome)
        // Controlla il primo nodo
        if (strcmp(current->nome, input) == 0) {
            *lista = current->next; // Il primo nodo è da eliminare
            free(current);
            printf("Prenotazione per %s eliminata.\n", input);
            puts("------------------------------------------");
            return;
        }

        // Scorri la lista per trovare il nodo da eliminare
        while (current != NULL) {
            if (strcmp(current->nome, input) == 0) {
                precedente->next = current->next;
                free(current);
                printf("Prenotazione per %s eliminata.\n", input);
                puts("------------------------------------------");
                return;
            }
            precedente = current;
            current = current->next;
        }
    }

    printf("Prenotazione non trovata.\n");
}

//funzione supporto a cancellazione
int is_numeric(char* str){
    while(*str){ // Continua finché *str non è '\0'
        if(!isdigit(*str)){
            return 0; //non è un numero
        }
        str++;
    }
    return 1; //tutte cifre
}

void LiberaMemoria(Nodo* lista){
    Nodo* current = lista;
    while(current != NULL){
        Nodo* prossimo = current->next;
        free(current);
        current = prossimo;
    }
    puts("Memoria liberata!");
}

void ScriviSuFile(){
    printf("Inserisci il nome del file sul quale vuoi salvare le prenotazioni: ");
    
}