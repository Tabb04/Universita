#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> // Per opendir, readdir, closedir
#include <errno.h>  // Per errno
#include <sys/stat.h> // Per stat() e S_ISREG() per una maggiore robustezza

#define OUTPUT_FILENAME "test.txt"
#define SEPARATOR_START "-------------------- CONTENUTO FILE --------------------\n"
#define SEPARATOR_END   "------------------ FINE CONTENUTO FILE -----------------\n\n"

// Funzione per controllare se il nome del file termina con .c o .h
int is_source_file(const char *filename) {
    const char *dot = strrchr(filename, '.'); // Trova l'ultima occorrenza di '.'
    if (dot) { // Se un punto è stato trovato
        if (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0) {
            return 1; // È un file .c o .h
        }
    }
    return 0; // Non è un file .c o .h
}

// Funzione per verificare se un path è un file regolare
int is_regular_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        // Errore nel chiamare stat, potrebbe essere un problema di permessi
        // o il file è stato rimosso nel frattempo.
        // Per questo esempio, lo trattiamo come se non fosse un file regolare.
        fprintf(stderr, "Attenzione: Impossibile ottenere informazioni su '%s': %s\n", path, strerror(errno));
        return 0;
    }
    return S_ISREG(path_stat.st_mode); // S_ISREG è una macro che restituisce vero se è un file regolare
}


int main() {
    FILE *output_file = fopen(OUTPUT_FILENAME, "w");
    if (!output_file) {
        perror("Errore: Impossibile creare/aprire il file di output test.txt");
        return EXIT_FAILURE;
    }

    DIR *dir_stream;
    struct dirent *dir_entry;
    int file_counter = 1;

    // Apre la directory corrente (".")
    dir_stream = opendir(".");
    if (!dir_stream) {
        perror("Errore: Impossibile aprire la directory corrente");
        fclose(output_file);
        return EXIT_FAILURE;
    }

    printf("Inizio elaborazione dei file nella directory corrente...\n");

    // Legge ogni entry nella directory
    while ((dir_entry = readdir(dir_stream)) != NULL) {
        const char *entry_name = dir_entry->d_name;

        // Ignora le directory "." e ".."
        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }

        // Verifica che sia un file regolare (non una directory, link simbolico, ecc.)
        // e che abbia l'estensione corretta.
        // Usare is_regular_file con stat() è più robusto di dir_entry->d_type
        // perché d_type non è garantito su tutti i filesystem.
        if (is_regular_file(entry_name) && is_source_file(entry_name)) {
            fprintf(output_file, "FILE %d: %s\n", file_counter, entry_name);
            fprintf(output_file, "%s", SEPARATOR_START);

            FILE *input_file = fopen(entry_name, "r");
            if (!input_file) {
                // Stampa errore su stderr (console) e anche nel file di log
                fprintf(stderr, "ATTENZIONE: Impossibile aprire il file sorgente '%s': %s\n", entry_name, strerror(errno));
                fprintf(output_file, "ERRORE: Impossibile leggere il file '%s'. Causa: %s\n", entry_name, strerror(errno));
            } else {
                printf("Leggendo e scrivendo: %s\n", entry_name);
                int ch;
                // Legge il file sorgente carattere per carattere e lo scrive nel file di output
                while ((ch = fgetc(input_file)) != EOF) {
                    if (fputc(ch, output_file) == EOF) {
                        perror("Errore grave: Impossibile scrivere su test.txt durante la copia del contenuto");
                        // In caso di errore di scrittura, è meglio interrompere
                        fclose(input_file);
                        fclose(output_file);
                        closedir(dir_stream);
                        return EXIT_FAILURE;
                    }
                }
                fclose(input_file);
            }
            // Assicura che ci sia un newline prima del separatore di fine,
            // utile se il file letto non terminava con un newline.
            fprintf(output_file, "\n%s", SEPARATOR_END);
            file_counter++;
        }
    }

    // Controlla se readdir è terminato a causa di un errore
    if (errno != 0 && dir_entry == NULL) {
        perror("Errore durante la lettura della directory");
    }

    closedir(dir_stream);
    fclose(output_file);

    if (file_counter == 1) {
        printf("Nessun file .c o .h trovato nella directory corrente.\n");
        // Potresti voler eliminare test.txt se è vuoto o scrivere un messaggio al suo interno
        // fopen(OUTPUT_FILENAME, "w"); // per sovrascriverlo come vuoto
        // fprintf(output_file, "Nessun file .c o .h trovato.\n");
        // fclose(output_file);
    } else {
        printf("Operazione completata. %d file sorgente processati.\n", file_counter - 1);
        printf("I risultati sono stati scritti in %s\n", OUTPUT_FILENAME);
    }

    return EXIT_SUCCESS;
}