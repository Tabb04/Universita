#define __FAVOR_BSD
#define _DEFAULT_SOURCE
//Aggiunte per evitare warning

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
#define ALARM_SLEEP 1   //Timer ogni quanto segnalare di stampare il riassunto

int verbose = 0;
pcap_dumper_t* dumper = NULL; //Funziona come handler per andare a scrivere i pacchetti su file
pcap_t* pd; //L'handler da dove leggerò i pacchetti



static struct timeval startTime;    //Tempo inizio del programma
unsigned long long numPkts = 0, numBytes = 0;   //Traccia il numero pacchetti e byte




/*                 FUNZIONI DI SUPPORTO                       */
/**************************************************************/


int32_t gmt_to_local(time_t t) {
  int dt, dir;
  struct tm *gmt, *loc;
  struct tm sgmt;

  if (t == 0)
    t = time(NULL);
  gmt = &sgmt;
  *gmt = *gmtime(&t);
  loc = localtime(&t);
  dt = (loc->tm_hour - gmt->tm_hour) * 60 * 60 +
    (loc->tm_min - gmt->tm_min) * 60;

  /*
   * If the year or julian day is different, we span 00:00 GMT
   * and must add or subtract a day. Check the year first to
   * avoid problems when the julian day wraps.
   */
  dir = loc->tm_year - gmt->tm_year;
  if (dir == 0)
    dir = loc->tm_yday - gmt->tm_yday;
  dt += dir * 24 * 60 * 60;

  return (dt);
}


/**************************************************************/



int drop_privilegi(const char* username){

    struct passwd *pw = NULL;   //Valore della password dell'utente

    //getgid e getuid restituiscono ID utente e ID gruppo che sta eseguendo il processo
    //Utende root è sempre utente 0
    if (getgid() && getuid()){
        fprintf(stderr, "Privilegi non droppati perché non sono superuser\n");
        return -1;
    }

    pw = getpwnam(username);    //recupera la password di "nobody"

    //se trovo nobody faccio il declassamento
    if (pw != NULL){
        if(setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0){     //cambio prima gruppo poi utente
            fprintf(stderr, "Impossibile cambiare privilegi\n");
            return -1;
        }else{
            printf("Utente cambiato a %s\n", username);
        }
    }else{
        fprintf(stderr, "Imposssibile trovare lo user %s\n", username);
        return -1;
    }
    umask(0);
    return 0;
}


/**************************************************************/


long delta_time (struct timeval * now, struct timeval * before){ //Serve a calcolare in microsecondi tra l'inizio del programma (startTime) e ora.  
    time_t delta_seconds;
    time_t delta_microseconds;

    /*
    compute delta in second, 1/10's and 1/1000's second units
    */
    delta_seconds      = now -> tv_sec  - before -> tv_sec;
    delta_microseconds = now -> tv_usec - before -> tv_usec;

    if(delta_microseconds < 0) {
    /* manually carry a one from the seconds field */
    delta_microseconds += 1000000;  /* 1e6 */
    -- delta_seconds;
    }
  
    return((delta_seconds * 1000000) + delta_microseconds);
}



/*                    PRINT_STATS                             */
/**************************************************************/
void stampa_stats(){

    static struct timeval lastTime;
    struct timeval endTime;     //Orario attuale
    float deltaSec; //Secondi totali trascorsi
    struct pcap_stat pcapStat;

    /*
    Se StartTime è 0 vuol dire che in dummyProcess non sono ancora arrivato
    in fondo, quindi non ho parsato ancora nessun pacchetto.
    */
    if(startTime.tv_sec == 0){  
        lastTime.tv_sec = 0;
        gettimeofday(&startTime, NULL);
        return;
    }

    gettimeofday(&endTime, NULL);
    deltaSec = (double)delta_time(&endTime, &startTime)/1000000;

    /*
    pcap_stats: dato un handle, riempie pcapStat con statistiche della cattura pacchetti (ricevuti, droppati, ecc)
    Popola pcapStats con:
        ps_recv (pacchetti ricevuti e che hanno passato il filtro), 
        ps_drop (pacchetti droppati dal kernel per buffer troppo pieno)
        ps_ifdrop (pacchetti droppati dall'interfaccia per congestione a livello fisico)
    */
    if(pcap_stats(pd, &pcapStat) >= 0){ 

    }

}






/**************************************************************/



void sigproc(int sig){
    static int called = 0;  //Uso una statica, così se l'utente spamma Ctrl+c, alle successive chiamate non fa nulla

    fprintf(stderr, "Uscita...\n");
    if (called) return;
    else called = 1;
    
    pcap_breakloop(pd); //DIce al ciclo di cattura di fermarsi al prossimo pacchetto ed uscire
}


/**************************************************************/


void mio_sigalarm(int sig){
    //stampa_stats()
    alarm(ALARM_SLEEP);
    signal(SIGALRM, mio_sigalarm);
}


/**************************************************************/


static char hex[] = "0123456789ABCDEF";
char* eth_addr_string(const unsigned char* ep, char* buf){  //Prende un puntatore ai byte dell'indirizzo e lo restituisce in esadecimale
    unsigned int i, j;
    char* cp;

    cp = buf;
    if ((j = *ep >> 4) != 0)
        *cp++ = hex[j];
    else
        *cp++ = '0';

    *cp++ = hex[*ep++ & 0xf];

    for(i = 5; (int)--i >= 0;) {
        *cp++ = ':';
        if ((j = *ep >> 4) != 0)
            *cp++ = hex[j];
        else
            *cp++ = '0';

        *cp++ = hex[*ep++ & 0xf];
    }

    *cp = '\0';
    return (buf);

}


/**************************************************************/


char* __intoa(unsigned int addr, char* buf, u_short bufLen){   //Rimpiazza inet_ntoa()
    char *cp, *retStr;
    u_int byte;
    int n;

    cp = &buf[bufLen];
    *--cp = '\0';

    n = 4;
    do{
        byte = addr & 0xff;
        *--cp = byte % 10 + '0';
        byte /= 10;
        if (byte > 0){
            *--cp = byte % 10 + '0';
            byte /= 10;
        if (byte > 0)
	    *--cp = byte + '0';
    }

    *--cp = '.';
    addr >>= 8;
    }while(--n > 0);

    /* Convert the string to lowercase */
    retStr = (char*)(cp+1);

    return(retStr);
}


/**************************************************************/


static char buf[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"];

char* intoa(unsigned int addr) {    //Trasforma il numero da 32 bit in testo
  return(__intoa(addr, buf, sizeof(buf)));
}


/**************************************************************/


static inline char* in6toa(struct in6_addr addr6){ //Traduce il numero da 64 bit in testo
    snprintf(buf, sizeof(buf),
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
	    addr6.s6_addr[0], addr6.s6_addr[1], addr6.s6_addr[2],
	    addr6.s6_addr[3], addr6.s6_addr[4], addr6.s6_addr[5], addr6.s6_addr[6],
	    addr6.s6_addr[7], addr6.s6_addr[8], addr6.s6_addr[9], addr6.s6_addr[10],
	    addr6.s6_addr[11], addr6.s6_addr[12], addr6.s6_addr[13], addr6.s6_addr[14],
	    addr6.s6_addr[15]);

    return(buf);
}


/**************************************************************/


char* proto2str(unsigned short proto){  //Converte il numero di protocollo in una stringa "TCP", "UDP" o "ICMP"
    static char protoName[8];

    switch(proto){
        case IPPROTO_TCP: return("TCP");
        case IPPROTO_UDP: return("UDP");
        case IPPROTO_ICMP: return ("ICMP");
        default:
            snprintf(protoName, sizeof(protoName), "%d", proto);
            return(protoName);
    }
}


/*              FUNZIONE DUMMY-PROCESS-PACKET                 */
/**************************************************************/


static int32_t thiszone;

void dummyProcessPacket(unsigned char* _deviceId, const struct pcap_pkthdr* h, const unsigned char* p){
    /*
    _deviceId -> putantore per dati utente non utilizzati
    h -> Struttura contenente i metadati del pacchetto (ora, lunghezza, ecc..)
    p -> Puntatore ai byte veri e propri del pacchetto intercettato
    */

    if(dumper){
        //Se avevo l'opzione di salvare su file, prende pacchetto grezzo e suoi metadati e li scrive su disco
        pcap_dump((unsigned char)dumper, (struct pcap_pkthdr*)h, p);    //tipo h per evitare warning
    }

    if(verbose){
        struct ether_header ehdr;   //Struttura dove mi copio l'header stesso
        unsigned short eth_type;    //Tipo di pacchetto ethernet (IPv4, IPv6, ARP, VLAN, ecc..)
        unsigned short vlan_id;     //Identificatore della VLAN
        char buf1[32], buf2[32];    //buffer per salvare l'indirizzo MAC destinatario/mittente in esadecimale
        struct ip ip;
        struct ip6_hdr ip6;


        int s = (h->ts.tv_sec + thiszone) % 86400;  //secondi passati dall'inizio del giorno
        printf("%02d:%02d:%02d.%06u", s/3600, (s/3600)/60, s%60, (unsigned)h->ts.tv_usec);  //Orario quando accade l'evento

        //Copio i primi byte dell'header in eth_head
        memcpy(&ehdr, p, sizeof(struct ether_header));  

        //Serve perché i byte a volte vengono letti in direzioni diverse (Big-Endian o Little-Endian)
        eth_type = ntohs(ehdr.ether_type);  //Salvo i byte nella direzione giusta in base al mio pc

        printf("[%s -> %s]", eth_addr_string(ehdr.ether_shost, buf1), eth_addr_string(ehdr.ether_dhost, buf2));

        //Se il pacchetto eth ha un tag VLAN, scorro di 4 byte per far finta che non ci sia la VLAN
        if(eth_type == 0x8100){
            vlan_id = (p[14] & 15)*256 + p[15];
            eth_type = (p[16])*256 + p[17];
            printf("[vlan %u] ", vlan_id);
            p+=4;
        }

        //CASO IPv4
        if(eth_type == 0x0800){
            memcpy(&ip, p+sizeof(ehdr), sizeof(struct ip));     //Mi copio l'intestazione
            printf("[%s]", proto2str(ip.ip_p)); //Prende un numero di protocollo e ritorna una stringa "TCP", "UDP", ecc..
            
            /*
            ip.ip_src.s_addr: Indirizzo sorgente
            nthol: Sempre per conversione da Big a Little Endian
            intoa: Trasformo l'indirizzo 32 bit una stringa testuale
            */
            printf("[%s ", intoa(ntohl(ip.ip_src.s_addr)));
            printf("-> %s] ", intoa(ntohl(ip.ip_dst.s_addr)));

        //CASO IPv6
        }else if(eth_type == 0x86DD){
            memcpy(&ip6, p+sizeof(ehdr), sizeof(struct ip6_hdr));   //Uguale che per IPv4
            printf("[%s ", in6toa(ip6.ip6_src));
            printf("-> %s] ", in6toa(ip6.ip6_dst));
        
        //CASO ARP
        }else if(eth_type == 0x0806){
            printf("[ARP]");

        //CASI NON ANALIZZATI
        }else{
            printf("[eth_type=0x%04X]", eth_type);
        }
        printf("[caplen=%u][len=%u]\n", h->caplen, h->len);
    }

    if(numPkts == 0) gettimeofday(&startTime, NULL);  //Se è il primo pacchetto parsato registro l'ora
    numPkts++, numBytes += h->len;  //Uso la lunghezza totale del pacchetto, mi serve per i Megabits/s

    //Ricordo che h->len è la lunghezza reale mentre h->caplen è ciò che ho copiato (senza payload di predefinito)

    if (verbose == 2){  //Scorro su tutti i byte di caplen e li stampo
        int i;

        for (i=0; i<h->caplen; i++){
            printf("%02X ", p[i]);
        }
        printf("\n");

    }

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
    struct bpf_program fcode;   //Struttura dove è salvato il bytecode

    bpf_u_int32 net;    //Indirizzo IP rilevato
    bpf_u_int32 mask;   //Maschera rilevata

    startTime.tv_sec = 0;
    thiszone = gmt_to_local(0);

    

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

    if(drop_privilegi("nobody") < 0){
        return (-1);
    }

    signal(SIGINT, sigproc);
    signal(SIGTERM, sigproc);
    
    /*
    Quando non sono in modalità verbosa uso un segnale SIGALARM per dire alla mia funzione mio_sigalarm
    di stampare i riassunti ogni secondo
    */

    if(!verbose){   
        signal(SIGALRM, mio_sigalarm);
        alarm(ALARM_SLEEP);
    }
    
    
    /*
    pcap_loop: Per ogni pacchetto che la libreria intercetta, esegue la funzione dummyProcessPacket
    passando alla funzione i dati nel pacchetto (vedi meglio sotto i parametri dentro la funzione)
    */
    pcap_loop(pd, -1, dummyProcessPacket, NULL);   
    
    //stampa_stats()


    pcap_close(pd); //Chiude la sessione di cattura

    if(dumper){     //Se avevo usato il flag -w lo chiude in modo sicuro
        pcap_dump_close(dumper);
    }

    return 0;

}