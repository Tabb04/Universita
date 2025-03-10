#!/bin/bash

#lista soltanto con i file della directory passata come argomento
#usa il comando findo epr listare i file nella directory passata come argomento
#nel seguente modo: find <directory> -maxdepth 1 -type f


#controllo se è stato passato un argomento

#posso usare if [ $# -lt 1 ]
if [ -z "$1" ]; then
    echo "Passa un argomento!"
    exit 1
fi

#verificac se l'argomento è una directory valida
if [ ! -d "$1" ]; then
    echo "Errore: $1 non è duna dirctory valida"
    exit 1
fi 

#lista solo i file escludendo le direcotory
echo "File nella directory '$1':"
find "$1" -maxdepth 1 -type f