#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "Syscalls.h"
#define VERSION2
//#undef VERSION2

#define FILENAME "test.txt"

int main(){
    int risultato;
    struct stat stat_file;
    DIR *directory1;
    struct dirent *corrente;
    
    #ifdef VERSION2
        SCALL(risultato, stat(FILENAME, &stat_file), "Errore durante stat");
        printf("Dimensione file: %ld bytes\n", stat_file.st_size);
        SNCALL(directory1, opendir("."), "Errore durante opendir");
        SNCALLREAD(corrente, readdir(directory1), printf("Trovato: %s\n", corrente->d_name), "Errore durante readdir");
        SCALL(risultato, closedir(directory1), "Errore nella chiusura");
    #else
        risultato = stat(FILENAME, &stat_file);

        if (risultato == -1){
            perror("Errore durante la stat");
            exit(EXIT_FAILURE);
        }
        printf("Dimensione file: %ld bytes\n", stat_file.st_size);

        directory1 = opendir(".");
        if (directory1 == NULL){
            perror("Errore durante opendir");
            exit(EXIT_FAILURE);
        }

        while((corrente = readdir(directory1)) != NULL){
            printf("Trovato: %s\n", corrente->d_name);
        }

        if((risultato = closedir(directory1)) == -1){
            perror("Errore durante closedir");
            exit(EXIT_FAILURE);
        }
    #endif
    return 0;
}
