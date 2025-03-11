#!/bin/bash

#vediamo se un file ha subito delle modifiche
#utilizzeremo DIFF per confrontare con il file attuale
#con un file di backup

#nella cartella test sotto

#come aggiungo una riga ad un fle testo?
#echo "ciao!" >> file.txt
#reinderizzo il flusso, append uso doppio maggiore




#prendo in input file
#controllo se esiste file con stesso come con suffisso bak
#se non esisto creo ed esco
#se esiste faccio l'else




#.. parte sopra
#controllo se passato un parametro
#CREDO SIA QUESTO RICONTROLLA
if [ -z "$1" ]; then
    echo "Inserisci un parametro che sia una directory"
    exit 1
fi

FILE="$1"       
BACKUP="${FILE}.bak"

#se il file di backup non eisste, crealo
if [ ! -f "$BACKUP" ]; then
    cp "$FILE" "$BACKUP"
    echo "Backup inziale creato per $FILE"
    exit 0
fi

#confronta il file attale con backup
diff "$BACKUP" "$FILE" > /dev/null                  #dev/null se non serve output buttalo
if [ $? -eq 0 ]; then                               #$? -> contiene il ritorno dell'ultimo comando o confronto di una guardia
    echo "Nessuna modifica rilevata in $FILE"       #quindi se diff non trova differenze da 0

else 
    echo "Modifile rilevate in $FILE:"
    diff -u "$BACKUP" "$FILE"
    cp "$FILE" "$BACKUP" #aggiorna il backup
fi