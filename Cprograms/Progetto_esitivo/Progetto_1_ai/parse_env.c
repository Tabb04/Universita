#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Funzione interna per rimuovere spazi bianchi iniziali e finali da una stringa.
 * Modifica la stringa in-place.
 * @param str La stringa da trimmare.
 */
static void trim_whitespace(char *str) {
    if (!str) return; // Sicurezza: non fare nulla se il puntatore è nullo
    char *end;

    // Trim leading space (Rimuovi spazi iniziali)
    // Finché il carattere puntato da 'str' è uno spazio bianco...
    while (isspace((unsigned char)*str)) str++; // ...incrementa il puntatore 'str'.
                                              // Questo sposta l'inizio logico della stringa.

    if (*str == 0) // Se la stringa conteneva solo spazi, ora punta al terminatore nullo.
        return;    // La stringa è effettivamente vuota, esci.

    // Trim trailing space (Rimuovi spazi finali)
    // 'end' punta all'ultimo carattere *prima* del terminatore nullo originale.
    end = str + strlen(str) - 1;
    // Finché 'end' è dopo l'inizio della stringa e punta a uno spazio bianco...
    while (end > str && isspace((unsigned char)*end)) end--; // ...decrementa 'end'.

    // Scrive un nuovo terminatore nullo dopo l'ultimo carattere non-spazio.
    // end[0] è l'ultimo carattere buono, end[1] è la posizione successiva.
    end[1] = '\0';
}

bool parse_environment(const char *filename, environment_t *env_config) {
    // 1. Validazione degli Input
    if (!filename || !env_config) {
        fprintf(stderr, "Errore: filename o puntatore env_config nullo passato a parse_environment.\n");
        return false; // Indica fallimento
    }

    // 2. Apertura del File
    FILE *file = fopen(filename, "r"); // Apre in modalità lettura ("r")
    if (!file) {
        // Se fopen fallisce (restituisce NULL), stampa un errore di sistema e uno personalizzato
        perror("Errore apertura file environment"); // Es: "Errore apertura file environment: No such file or directory"
        fprintf(stderr, "Impossibile aprire il file di configurazione ambiente: %s\n", filename);
        return false; // Indica fallimento
    }

    // 3. Inizializzazione Variabili Locali
    char line[MAX_LINE_LENGTH]; // Buffer per leggere una riga alla volta
    int line_num = 0;           // Contatore numero riga (per messaggi di errore)
    bool queue_found = false;   // Flag per verificare se abbiamo trovato la riga "queue="
    bool height_found = false;  // Flag per verificare se abbiamo trovato la riga "height="
    bool width_found = false;   // Flag per verificare se abbiamo trovato la riga "width="

    // Inizializza la struttura di output con valori non validi o vuoti.
    // Utile se il file è incompleto.
    env_config->queue_name[0] = '\0'; // Stringa vuota
    env_config->grid_height = -1;    // Valore invalido (assumendo che l'altezza debba essere positiva)
    env_config->grid_width = -1;     // Valore invalido (assumendo che la larghezza debba essere positiva)

    // 4. Ciclo di Lettura del File
    // Legge una riga alla volta finché fgets non restituisce NULL (fine file o errore)
    while (fgets(line, sizeof(line), file)) {
        line_num++; // Incrementa il numero di riga

        // Rimuove il carattere newline ('\n') che fgets include alla fine della riga (se c'è spazio)
        line[strcspn(line, "\n")] = 0;
        // strcspn(line, "\n") trova l'indice del primo '\n' in 'line'.
        // Mettendo '\0' a quell'indice, la stringa viene troncata lì.

        // Pre-processamento della riga letta
        char *trimmed_line = line;   // Usa un puntatore temporaneo
        trim_whitespace(trimmed_line); // Rimuove spazi bianchi iniziali/finali

        // Ignora righe vuote o righe che iniziano con '#' (commenti)
        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#') {
            continue; // Salta al prossimo ciclo (prossima riga)
        }

        // 5. Parsing della Riga (Formato Chiave=Valore)
        // strtok divide la stringa in "token" basandosi sul delimitatore "=".
        // IMPORTANTE: strtok modifica la stringa originale (trimmed_line)!
        char *key = strtok(trimmed_line, "=");
        char *value = strtok(NULL, "="); // La chiamata successiva con NULL continua dalla posizione precedente

        if (key && value) { // Se abbiamo trovato sia una chiave che un valore...
            // Pulisci ulteriormente chiave e valore da eventuali spazi rimasti attaccati
            trim_whitespace(key);
            trim_whitespace(value);

            // 6. Identificazione e Memorizzazione dei Valori
            // Confronta la chiave (ignorando maiuscole/minuscole sarebbe più robusto, ma qui usiamo strcmp)
            if (strcmp(key, "queue") == 0) {
                // Copia il valore nella struttura. Usa strncpy per sicurezza contro buffer overflow.
                strncpy(env_config->queue_name, value, sizeof(env_config->queue_name) - 1);
                // Assicura che la stringa sia sempre terminata da '\0', perché strncpy non lo garantisce
                // se la sorgente è più lunga del buffer di destinazione.
                env_config->queue_name[sizeof(env_config->queue_name) - 1] = '\0';
                queue_found = true; // Imposta il flag
            } else if (strcmp(key, "height") == 0) {
                // Prova a convertire il valore stringa in un intero.
                // sscanf restituisce il numero di conversioni riuscite (deve essere 1).
                // Controlla anche che il valore sia positivo.
                if (sscanf(value, "%d", &env_config->grid_height) != 1 || env_config->grid_height <= 0) {
                    fprintf(stderr, "Errore parsing environment: Valore 'height' non valido (%s) alla riga %d\n", value, line_num);
                    fclose(file); // Chiudi il file prima di uscire per errore
                    return false; // Fallimento
                }
                height_found = true; // Imposta il flag
            } else if (strcmp(key, "width") == 0) {
                // Come per 'height'
                if (sscanf(value, "%d", &env_config->grid_width) != 1 || env_config->grid_width <= 0) {
                    fprintf(stderr, "Errore parsing environment: Valore 'width' non valido (%s) alla riga %d\n", value, line_num);
                    fclose(file);
                    return false; // Fallimento
                }
                width_found = true; // Imposta il flag
            } else {
                // Chiave non riconosciuta: stampa un avviso ma non interrompere il parsing.
                fprintf(stderr, "Attenzione: Chiave sconosciuta '%s' nel file environment alla riga %d\n", key, line_num);
            }
        } else {
            // La riga non era nel formato "chiave=valore" atteso dopo il trim.
            fprintf(stderr, "Errore parsing environment: Formato linea non valido alla riga %d: '%s'\n", line_num, line);
            // Decidiamo di non terminare per questo errore, magari è solo una riga malformata.
        }
    } // Fine del ciclo while (fgets)

    // 7. Chiusura del File
    fclose(file); // Molto importante rilasciare le risorse associate al file.

    // 8. Validazione Finale
    // Controlla se tutti i parametri obbligatori sono stati trovati e validati
    if (!queue_found || !height_found || !width_found) {
        fprintf(stderr, "Errore parsing environment: Uno o più parametri obbligatori (queue, height, width) mancanti nel file %s.\n", filename);
        return false; // Fallimento
    }
    // Controlla specificamente se il nome della coda è vuoto (potrebbe succedere se la riga era "queue=")
     if (strlen(env_config->queue_name) == 0) {
         fprintf(stderr, "Errore parsing environment: Il nome della coda ('queue') non può essere vuoto.\n");
         return false; // Fallimento
     }

    // 9. Successo
    // Stampa un riepilogo della configurazione letta (utile per debug)
    printf("Configurazione ambiente letta:\n");
    printf("  Queue: %s\n", env_config->queue_name);
    printf("  Height: %d\n", env_config->grid_height);
    printf("  Width: %d\n", env_config->grid_width);

    return true; // Indica successo
}