#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "Syscalls_2.h"

#define BUFFER_SIZE 128
#define FILENAME "dati.txt"

int main(){
    int fd;
    int ris;

    SCALL(fd, open(FILENAME, O_RDONLY), "Errore read");

    char buffer[BUFFER_SIZE];
    ssize_t byte_letti;
    //invece di sizeof(buffer) anche BUFFER_SIZE
    SCALLREAD(byte_letti, read(fd, buffer, sizeof(buffer)), write(STDOUT_FILENO, buffer, byte_letti), "Errore read");

    SCALL(ris, close(fd), "Errore close");
    return 0;
}