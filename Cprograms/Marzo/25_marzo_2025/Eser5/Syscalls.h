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



#define PARENT_OR_CHILD(pid,f_parent,f_child)\
    do {\
        if(pid == 0){\
            f_child;\
        } else {\
            f_parent;\
        }\
    }while(0)\


#endif // ERROR_HANDLING_H