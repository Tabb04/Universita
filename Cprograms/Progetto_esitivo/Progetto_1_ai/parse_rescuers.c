#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // Per errno
#include <ctype.h> // Per isspace

// Funzione helper per liberare la lista in caso di errore durante il parsing
void free_rescuer_type_list(rescuer_type_t *head) {
    rescuer_type_t *current = head;
    rescuer_type_t *next;
    while (current != NULL) {
        next = current->next;
        // Il nome non è allocato dinamicamente qui perché usiamo un array fisso
        free(current);
        current = next;
    }
}

bool parse_rescuers(const char *filename, rescuer_type_t **rescuer_type_list, int *total_rescuers_count) {
    if (!filename || !rescuer_type_list || !total_rescuers_count) {
        fprintf(stderr, "Errore: Argomenti nulli passati a parse_rescuers.\n");
        return false;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file rescuers");
        fprintf(stderr, "Impossibile aprire il file di configurazione soccorritori: %s\n", filename);
        return false;
    }

    *rescuer_type_list = NULL; // Inizializza la lista come vuota
    *total_rescuers_count = 0;
    rescuer_type_t *current = NULL;
    rescuer_type_t *tail = NULL; // Puntatore all'ultimo nodo per aggiunte O(1)

    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0; // Rimuove newline

        // Ignora righe vuote o commenti
        char *trimmed_line = line;
        while(isspace((unsigned char)*trimmed_line)) trimmed_line++;
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }

        // Expected format: [<name>] [<num>] [<speed>] [<x>;<y>]
        char name[RESCUER_NAME_MAX_LEN];
        int num, speed, base_x, base_y;

        // Usiamo sscanf per estrarre i valori. Nota: %[^]] legge fino a ']'
        // Lo spazio dopo %d è importante per consumare eventuali spazi bianchi
        int matched = sscanf(trimmed_line, " [%[^]]] [%d ] [%d ] [%d;%d ]",
                             name, &num, &speed, &base_x, &base_y);

        if (matched != 5) {
            fprintf(stderr, "Errore parsing rescuers: Formato linea non valido alla riga %d: '%s'\n", line_num, trimmed_line);
            fprintf(stderr, "Formato atteso: [<name>] [<num>] [<speed>] [<x>;<y>]\n");
            free_rescuer_type_list(*rescuer_type_list); // Libera la memoria allocata finora
            *rescuer_type_list = NULL;
            fclose(file);
            return false;
        }

        // Validazione dei valori letti
        if (num <= 0 || speed <= 0 || base_x < 0 || base_y < 0) {
             fprintf(stderr, "Errore parsing rescuers: Valori numerici non validi (num=%d, speed=%d, base_x=%d, base_y=%d) alla riga %d\n",
                     num, speed, base_x, base_y, line_num);
             free_rescuer_type_list(*rescuer_type_list);
             *rescuer_type_list = NULL;
             fclose(file);
             return false;
        }
         // Validazione nome (non vuoto)
        if (strlen(name) == 0 || strlen(name) >= RESCUER_NAME_MAX_LEN) {
            fprintf(stderr, "Errore parsing rescuers: Nome soccorritore non valido o troppo lungo alla riga %d\n", line_num);
            free_rescuer_type_list(*rescuer_type_list);
            *rescuer_type_list = NULL;
            fclose(file);
            return false;
        }

        // Crea un nuovo nodo per il tipo di soccorritore
        current = (rescuer_type_t *)malloc(sizeof(rescuer_type_t));
        if (!current) {
            perror("Errore allocazione memoria per rescuer_type_t");
            free_rescuer_type_list(*rescuer_type_list);
            *rescuer_type_list = NULL;
            fclose(file);
            return false;
        }

        // Copia i dati nel nuovo nodo
        strncpy(current->name, name, RESCUER_NAME_MAX_LEN - 1);
        current->name[RESCUER_NAME_MAX_LEN - 1] = '\0'; // Assicura null termination
        current->total_count = num;
        current->speed = speed;
        current->base_x = base_x;
        current->base_y = base_y;
        current->next = NULL;

        // Aggiunge il nodo alla lista linkata
        if (*rescuer_type_list == NULL) {
            *rescuer_type_list = current; // Primo nodo
            tail = current;
        } else {
            tail->next = current; // Aggiunge in coda
            tail = current;       // Aggiorna la coda
        }

        // Aggiorna il contatore totale dei soccorritori
        *total_rescuers_count += num;

        printf("  Letto tipo soccorritore: %s (Num: %d, Speed: %d, Base: %d;%d)\n",
               current->name, current->total_count, current->speed, current->base_x, current->base_y);
    }

    fclose(file);

    if (*rescuer_type_list == NULL) {
         fprintf(stderr, "Attenzione: Nessun tipo di soccorritore definito nel file %s.\n", filename);
         // Potrebbe essere valido o meno a seconda dei requisiti, per ora non è un errore fatale.
    }

    printf("Parsing soccorritori completato. Tipi letti: %d. Totale mezzi: %d\n", line_num, *total_rescuers_count); // line_num qui conta le righe valide
    return true;
}
