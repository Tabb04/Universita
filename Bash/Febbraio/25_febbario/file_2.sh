#!/bin/bash

numero=0        #inizializzo

#Continua a chiedere un numero fino a che è >10
while [ "$numero" -le 10]; do
    read -p "Inserisci un numero >10: " numero
done

echo "Hai inserito un numero >10"

#Chiede un altro numero

...