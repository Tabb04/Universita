#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> // Per opendir, readdir, closedir
#include <errno.h>  // Per errno
#include <sys/stat.h> // Per stat() e S_ISREG()

#define OUTPUT_FILENAME "test.txt"
#define SEPARATOR_START "-------------------- CONTENUTO FILE --------------------\n"
#define SEPARATOR_END   "------------------ FINE CONTENUTO FILE -----------------\n\n"
#define SELF_FILENAME "salva_testo.c" // Nome del file del programma stesso

// Funzione per controllare se il nome del file termina con .c o .h
int is_source_file(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (dot) {
        if (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0) {
            return 1;
        }
    }
    return 0;
}

// Funzione per verificare se un path è un file regolare
int is_regular_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "Attenzione: Impossibile ottenere informazioni su '%s': %s\n", path, strerror(errno));
        return 0;
    }
    return S_ISREG(path_stat.st_mode);
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

    dir_stream = opendir(".");
    if (!dir_stream) {
        perror("Errore: Impossibile aprire la directory corrente");
        fclose(output_file);
        return EXIT_FAILURE;
    }

    printf("Inizio elaborazione dei file nella directory corrente...\n");

    while ((dir_entry = readdir(dir_stream)) != NULL) {
        const char *entry_name = dir_entry->d_name;

        if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
            continue;
        }

        // ***** NUOVO CONTROLLO: IGNORA IL FILE DEL PROGRAMMA STESSO *****
        if (strcmp(entry_name, SELF_FILENAME) == 0) {
            printf("Ignorando il file del programma stesso: %s\n", entry_name);
            continue; // Passa alla prossima entry della directory
        }
        // ******************************************************************

        if (is_regular_file(entry_name) && is_source_file(entry_name)) {
            fprintf(output_file, "FILE %d: %s\n", file_counter, entry_name);
            fprintf(output_file, "%s", SEPARATOR_START);

            FILE *input_file = fopen(entry_name, "r");
            if (!input_file) {
                fprintf(stderr, "ATTENZIONE: Impossibile aprire il file sorgente '%s': %s\n", entry_name, strerror(errno));
                fprintf(output_file, "ERRORE: Impossibile leggere il file '%s'. Causa: %s\n", entry_name, strerror(errno));
            } else {
                printf("Leggendo e scrivendo: %s\n", entry_name);
                int ch;
                while ((ch = fgetc(input_file)) != EOF) {
                    if (fputc(ch, output_file) == EOF) {
                        perror("Errore grave: Impossibile scrivere su test.txt durante la copia del contenuto");
                        fclose(input_file);
                        fclose(output_file);
                        closedir(dir_stream);
                        return EXIT_FAILURE;
                    }
                }
                fclose(input_file);
            }
            fprintf(output_file, "\n%s", SEPARATOR_END);
            file_counter++;
        }
    }

    if (errno != 0 && dir_entry == NULL) {
        perror("Errore durante la lettura della directory");
    }

    closedir(dir_stream);
    fclose(output_file);

    if (file_counter == 1) {
        printf("Nessun file .c o .h (diverso da %s) trovato nella directory corrente.\n", SELF_FILENAME);
    } else {
        printf("Operazione completata. %d file sorgente processati.\n", file_counter - 1);
        printf("I risultati sono stati scritti in %s\n", OUTPUT_FILENAME);
    }

    return EXIT_SUCCESS;
}