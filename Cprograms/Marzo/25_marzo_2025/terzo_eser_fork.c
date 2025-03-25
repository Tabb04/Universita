#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h> // fork e getpid

#define SCALL_ERROR -1
#define SCALL(r, c, e) do { if ((r = c) == SCALL_ERROR) {perror(e); exit(EXIT_FAILURE); }} while(0)
#define PARENT_OR_CHILD(pid, f_parent, f_child) do {if(pid == 0) {f_child;} else {f_parent}} while(?)


int parent(int pid){;}

int child(){;}

int main(){
    pid_t pid;
    SCALL(pid, fork(), "durante fork");

    PARENT_OR_CHILD(pid,
        parent(pid),
        child()
    );

    return 0;
}