#!/bin/bash

# hello.sh
# Saluto
nome = "Mondo"

# se è stato passato un argomento, lo assegno alla variabile nome
if [ $# -gt 0]; then # greater then
    nome =$1 #manca qualcosa
fi


#numero parametri
echo $#


#stampo nome se passato
echo "Ciao $nome!"