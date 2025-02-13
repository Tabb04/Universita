//alberi binari di ricerca

//albero è grafo binario aciclico
//tutti i sottoalberi sinistri hanno nodo inferiore, destri maggiore. Ricorsivamente


//simile alla linked list

typedef struct Nodo{
    void* chiave;
    struct Nodo* left;
    struct Nodo* right;
}Nodo;

//albero di altezza minima è quello pieno
//in media sui totalmente bilanciati è log(N)