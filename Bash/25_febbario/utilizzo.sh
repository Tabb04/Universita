#!/bin/bash

while true; do
    echo "Utilizzo CPU:"
    top -bn1 | grep "Cpu(s)"        # prende la prima riga
    sleep 5
done