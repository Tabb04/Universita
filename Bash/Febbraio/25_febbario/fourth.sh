#!/bin/bash

# hello.sh
# Saluto
nome="Mondo" #nome default

# se è stato passato un argomento, lo assegno alla variabile nome
if [ $# -gt 0 ]; then # greater then
    nome=$1 
fi


#numero parametri
echo "Numero parametri:" $#


#stampo nome se passato
echo "Ciao $nome!"