#!/bin/bash

# Verifica che sia stato passato un argomento
if [ -z "$1" ]; then
    echo "Per favore, fornisci una directory come argomento."
    exit 1
fi

# Controllo se la directory passata esiste
if [ ! -d "$1" ]; then                              # se NON directory (! -d)
    echo "Directory non esistente"
    exit 1
fi

# Per ogni file nella directory passata
for file in "$1"/*; do                              # for each file nella directory (*)
    # controllo sia un file                         #pk mostra directory corrente e padre (scrive . e ..)
    if [ -f "$file" ]; then
        nome=$(basename "$file")                        #estraggo nome file
        dimensione=$(stat -c %s "$file")                #dimensione file in byte
    fi
done

   ... 