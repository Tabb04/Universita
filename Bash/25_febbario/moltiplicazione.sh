#!/bin/bash
read -p "Inserisci un numero: " num

for i in {1..10}; do
    echo "$num moltiplicato $i = $((num * i))"          # senza $
done