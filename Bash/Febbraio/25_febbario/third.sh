#!/bin/bash

# Dichiaro una funzione, $1 se è in funzione rappresenta il primo parametro
function saluta {
    echo "Ciao, $1!"
}

# utilizzo della funzione
saluta "Mario"


# qua $1 signigica cosa passato a linea di comando 
echo "$1"
echo "$2"

# faccio third.sh Gigi Alberto stampa Ciao, Mario! Gigi Alberto
# $0 è il nome della funzione

