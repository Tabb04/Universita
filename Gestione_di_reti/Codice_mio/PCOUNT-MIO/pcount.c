#include <sys/types.h>


#include <pcap/pcap.h>
#include <sys/stat.h>
#include <signal.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>


#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>

//aggiunto perché a compilazione non mi trovava optarg
#include <bits/getopt_core.h>

#define DEFAULT_MASK 0xFFFFFF00  //Predefinita /24

int verbose = 0;
pcap_dumper_t* dumper = NULL; //Funziona come handler per andare a scrivere i pacchetti
pcap_t* pd; //L'handler da dove leggerò i pacchetti



int drop_privilegi(const char* username){
    
}





void stampa_aiuto(){
    char bufferr[PCAP_ERRBUF_SIZE]; //valore default settato a 256
    
    //pcap_if è una lista linkata con l'elenco delle interfacce di rete disponibili sul sitema
    pcap_if_t *puntatore_dev;

    printf("Utilizzo: pcount [-h] -i <device|path> [-w <path>] [-f <filter>] [-l <len>] [-v <1|2>]\n");
    printf("-h               [Print help]\n");
    printf("-i <device|path> [Device name or file path]\n");    //può essere un device oppure un file .pcap di catture già fatte
    printf("-f <filtro>      [pcap filter]\n");   //applica un filtro  
    printf("-w <path>        [pcap write file]\n");  //salva i pacchetti catturati in un file .pcap
    printf("-l <len>         [Capture length]\n");  //serve per catturare più dei byte di intestazione
    printf("-v <modalità>    [Verbose [1: verbose, 2: very verbose (print payload)]]\n");   //verboso


    //La funzione cerca tutti i device disponibili e ritorna una lista linkata di tipo pcap_if_t. Usa il buffer se ci sono errori
    if (pcap_findalldevs(&puntatore_dev, bufferr) == 0){
        int i = 0;

        printf("\nDevice disponibili (-i):\n");
        while(puntatore_dev){
            const char* desc = puntatore_dev->description;
        
            if (desc){
                printf(" %d. %s [%s]\n", i++, puntatore_dev->name, desc);
            }else{
                printf(" %d. %s\n", i++, puntatore_dev->name);
            }

            puntatore_dev = puntatore_dev->next;
        }
    }
}




int main(int argc, char* argv[]){

    unsigned char c;
    char* device = NULL;
    char* filtrobpf = NULL;
    int snaplung = 256;
    struct stat stats;
    char errbuf[PCAP_ERRBUF_SIZE];
    int promisc;
    struct bpf_program fcode;

    bpf_u_int32 net;    //Indirizzo IP rilevato
    bpf_u_int32 mask;   //Maschera rilevata
    

    //Scorre le flags passate. Le flag possibili sono: h, i, l, v, f, w
    //Se hanno ":" dopo vuol dire che insieme al flag va passato un valore
    //Se non è trovato uno di quei caratteri getopt restituisce '?'
    while((c = getopt(argc, argv, "hi:l:v:f:w:")) != '?'){
        if ((c == 255) || (c == (unsigned char)-1)) break;  //quando ha letto tutti i flag restituisce -1

        switch(c){

        case 'h':
            stampa_aiuto();
            exit(0);
            break;
        case 'i':

            //optarg è un puntatore. Quando getopt scorre sugli argv, arriva a "-i", poi setta optarg al valore dopo "-i"
            //quindi uso string-duplicate per salvarmela
            device = strdup(optarg);
            break;
        case 'l':
            //atoi: converte stringa ad intero
            snaplung = atoi(optarg);
            break;
        case 'v':
            verbose = atoi(optarg);
            break;
        case 'w':
            /*
            pcap_open_dead: 
                In questo momento non ho ancora aperto la scheda di rete, allora creo in memoria una struttura dati
                per il file dove scriverò i pacchetti salvati.
                Gli passo DLT_EN10MB per dire che sono pacchetti ethernet e 12384 che è la MTU (maximum trasmission unit)

            pcap_dump_open:
                Crea fisicamente il file ".pcap".
                Gli passo il contesto di cattura da pcap_open_dead e "optarg", che come visto sopra punta all'argomento
                (in questo caso il percorso del file di cattura)    
            
            Uso per salvare un puntatore di tipo pcap_dumper_t.
            
            */
            
            dumper = pcap_dump_open(pcap_open_dead(DLT_EN10MB, 16384), optarg);
            if(dumper == NULL) {
                printf("Impossibile aprire il file di dump %s\n", optarg);
                return(-1);
            }
            
           break;
        case 'f':
            //Salvo il nome del filtro
            filtrobpf = strdup(optarg);
            break;

        }
    }
 
    if (getuid() != 0){
        //Se non sono in sudo
        //Mi serve perché devo ascoltare in modalità promisqua (anche dati non diretti al mio pc)
        printf("Per favore esegui come superuser\n");
        return(-1);
    }
    //In pcount_capabilities ho usato CAP_NET_RAW e CAP_NET_ADMIN per avere controllo più granulare sui controlli da superuser


    if(device == NULL){
        printf("ERRORE: Missing -i\n");
        stampa_aiuto();
        return(-1);
    }

    printf("Catturando dal device %s\n", device);

    //stat: prende gli attributi di "device" e li mette nel buffer "stats"
    //Se restituisce 0 vuol dire che è un file su disco.
    if (stat(device, &stats) == 0){
        
        //pcap_open_offline (di libreria): Apre il file .pcap e restituisce un puntatore ad una "Sessione pcap" 
        if((pd = pcap_open_offline(device, errbuf)) == NULL){
            printf("pcap_open_offline: %s\n", errbuf);
            return(-1);
        }

        //Se leggo da file presuppongo una rete /24
        mask = DEFAULT_MASK;

    }else{
        //È un dispositivo
        promisc = 1;
        //pcap_open_live: passo il device, la lunghezza di quanto voglio leggere, 1 perché in modalità promisqua
        //500 per 500ms e il buffer di errrore.
        if((pd = pcap_open_live(device, snaplung, promisc, 500, errbuf)) == NULL){
            printf("pcap_open_live: %s\n", errbuf);
            return(-1);
        }
    }

    /* Cerco quale è la maschera giusta */
    if(pcap_lookupnet(device, &net, &mask, errbuf) == -1){
        fprintf(stderr, "Maschera di rete non trovata per %s: %s\n", device, errbuf);
        mask = DEFAULT_MASK;
    }


    /*
    I filtri BPF funzionano allo stesso modo che i comandi testuali su wireshark del tipo
    "tcp port 80" o "host 192.168.1.4".
    */

    if(filtrobpf != NULL){
        /*
        pcap_compile: Traduce le espressioni letterali in bytecode
            Passo l'handler, una struttura dati dove salva il codice binario tradotto,
            il filtro, "1" per dire di ottimizzare e la maschera di rete (255.255.255.0).
            Restituisce un valore <0 se non legge qualcosa di valido.
        */
        if(pcap_compile(pd, &fcode, filtrobpf, 1, mask) < 0){
            printf("pcap_compile_error: '%s'\n", pcap_geterr(pd));
        }else{
            //Setto il filtro tradotto
            if(pcap_setfilter(pd, &fcode) < 0){
                printf("errore pcap_setfilter: %s\n", pcap_geterr(pd));
            }
        }
    }
    


    


}