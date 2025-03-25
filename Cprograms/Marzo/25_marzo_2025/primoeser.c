#include 



#define BUFFER_SIZE 128
#define FILENAME "test.txt"

#define SCALL_ERROR -1 
#define SCALL(r, c, e) do { if ((r = c) == SCALL_ERROR) {perror(e); exit(EXIT_FAILURE)}}

//perchè dentro do while??