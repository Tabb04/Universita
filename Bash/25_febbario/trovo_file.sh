#!/bin/bash
read -p "Inserisci la directory: " dir              # scrivi la frase e memorizza il risultato in dir

if [ -d "$dir" ]; then                               # se argomento è una directory
    max_file=$(ls -S "$dir" | head -n 1)             # valore è il risultato di ls su directory, prendi il primo risultato
    echo "Il file più grande è: $max_file"
else
    echo "Directory non trovata."
fi

# . per directory corrente