#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdio.h>
#include <stdlib.h>

#define SCALL_ERROR -1

#define SCALL(r, c, e) do { if((r = c) == SCALL_ERROR) { perror(e); exit(EXIT_FAILURE); } } while(0)

#define SNCALL(r, c, e) do { if((r = c) == NULL) { perror(e); exit(EXIT_FAILURE); } } while(0)

#define SNCALLREAD(a, b, c, d) \
    do{ \
        while((a = b) > 0){ \
            c; \
        }\
        if (a == -1){ \
            perror(d);\
            exit(EXIT_FAILURE);\
        }\
    }while(0)

#define SNCALLREAD(a,loop_cond_op,read_loop_op,e) \
    do{\
        while ((a = loop_cond_op) != NULL){\
            read_loop_op;\
        }\
        if (errno == SCALL_ERROR){\
            perror(e);\
            exit(EXIT_FAILURE);\
        }\
    }while(0)\


#endif // ERROR_HANDLING_H