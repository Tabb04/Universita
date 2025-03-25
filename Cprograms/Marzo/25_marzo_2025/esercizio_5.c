

//define e macro



int padre(){;}
int figlio(){;}

int main(){

    pid_t pid;

    //creo un processo figlio
    SCALL(pid, fork(), "durante fork");
    

}