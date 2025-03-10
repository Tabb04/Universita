#!/bin/bash

FILE="file.txt"
PAROLA="errore"

#verifica che il file esiste
if [ ! -f "$FILE" ]; then
    echo "Il file $FILE non esiste"
    exit 1
fi

echo "Monitoraggio di $FILE per la parola '$PAROLA'. Premere control c per fermare"

#loop infinito
while true; do
    sleep 2 #aspetto 2 secondi

    #cerca la paroal nel file
    if greep -q "$PAROLA" "$FILE"; then
    echo "La parola '$PAROLA' è stata trovata in $FILE"
    fi
done