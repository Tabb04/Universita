#!/bin/bash

if [ -z "$1" ]; then
    echo "Inserisci un parametro"
    exit 1
fi

if [ ! -f "$1" ]; then
    echo "Inserisci un file valido"
    exit 1
fi

FILE="$1"
BACKUP="${FILE}.bak"

if [ ! -f "$BACKUP" ]; then
    cp "$FILE" "$BACKUP"
    echo "Backup creato per $FILE"
    exit 0
fi

diff "$BACKUP" "$FILE" > /dev/null
if [ $? -eq 0 ]; then
    echo "File non modificato dal backup"
    exit 1
else
    echo "Modifiche rilevate"
    diff -u "$BACKUP" "$FILE"
    cp "$FILE" "$BACKUP"
    exit 0
fi

