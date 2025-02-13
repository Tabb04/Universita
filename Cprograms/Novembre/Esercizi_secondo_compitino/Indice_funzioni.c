//FUNZIONI

//legge una riga e memorizza aggiungendo \0
fgets(dove memorizzo, quanti caratteri leggo, da dove leggo)

//scrive dati formattati
fprintf(dove scrivo, cosa scrivo e formato tipo "Nome: %s\n", cosa scrivo tipo nome)

//scrive un blocco di dati binari in uno stream
fwrite(dati da scrivere, dimensione di ogni elemento, num elmenti, dove scrivere);

//legge i dati formattati e restituisce quanti letti
int fscanf(dove leggo, tipo da leggere %d %s, dove memorizzo i dati cioè un elenco di variabili)

//sposto il puntatore del flusso
seek(flusso, di quanto sposto, da dove inizio) //SEEK_SEt è inizio, SEEK_CUR è dove sono, SEEK_END fine

//
getline(dove metto la stringa, dimensione riga, dove prendo)

