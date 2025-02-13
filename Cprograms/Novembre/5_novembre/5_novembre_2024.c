//ALTRI ESEMPI DI LETTURA 
//FPUTS, FPUTC, FPRINTF, FREAD, FWRITE
//fflush e setbuf
//fseek


//fputs inserisce una stringa alla volta
file = fopen("esempio.txt", "w");
if (file == NULL) {
    perror("Errore nell'apertura del file");
    return 1;
    }

// Scrittura di una stringa
const char *testo = "Questa è una stringa scritta con fputs.";
if (fputs(testo, file) == EOF) {
    perror("Errore nella scrittura");
    fclose(file);
    return 1;
}
// Aggiunta di un'altra stringa con un ritorno a capo
fputs("\nAggiungiamo un'altra riga al file.\n", file);

//dentro esempio.txt ci sarà:
/*Questa è una stringa scritta con fputs.
Aggiungiamo un'altra riga al file.
*/




//Esempio fputc che inserisce lettera per lettera
file = fopen("output.txt", "w");
if (file == NULL) {
    perror("Errore nell'apertura del file");
    return 1;
}

const char *stringa = "Ciao mondo!";
for (int i = 0; stringa[i] != '\0'; i++) {
        fputc(stringa[i], file); 
}

//nel file output.txt ci sarà "Ciao mondo!"




//esempio fprintf che inserisce dati formattati
file = fopen("dati.txt", "w");
if (file == NULL) {
    perror("Errore nell'apertura del file");
    return 1;
}

// Scrittura di dati formattati nel file
const char *nome = "Mario";
int eta = 25;
float altezza = 1.75;

fprintf(file, "Nome: %s\n", nome);         // Scrive una stringa
fprintf(file, "Età: %d\n", eta);          // Scrive un intero
fprintf(file, "Altezza: %.2f metri\n", altezza);

//in dati.txt ci sarà 
Nome: Mario
Età: 25
Altezza: 1.75 metri

//fread prende parametri (puntatore dove memorizzare, dimensione in byte di ogni elemento da leggere,
//                          numero di elementi da leggere, puntatore dove leggere)
                            


elementi_letti = fread(buffer, sizeof(int), 5, file);
    if (elementi_letti != 5) {
        if (feof(file)) {
            printf("Fine del file raggiunta.\n");
        } else {
            perror("Errore nella lettura del file");
        }
    }

    // Visualizza i dati letti
    printf("Dati letti dal file:\n");
    for (size_t i = 0; i < elementi_letti; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");

//restituisce anche il numero di valori letti



//Esempio fwrite
    int dati[] = {10, 20, 30, 40, 50}; // Dati da scrivere nel ile
    size_t elementi_scritti;

elementi_scritti = fwrite(dati, sizeof(int), 5, file);
if (elementi_scritti != 5) {
        perror("Errore nella scrittura del file");
        fclose(file);
        return 1;
}

//restituisce elementi scritti



int fflush ( FILE * stream );
//su stream aperti in modalità ‘w’ forza la scrittura svuotando il buffer
//chiamata da fclose

void setbuf ( FILE * stream, char * buffer );
//Assegna buffer allo stream che diventa fully buffered. Se buffer è NULL
//lo stream diventa unbuffered.


//fseek serve per spostare dove punta il buffer una volta aperto

int fseek(FILE *fp, long distanza, int partenza);

//per fare partenza inizio file faccio SEEK_SET
//per fare fine del file uso SEEK_END