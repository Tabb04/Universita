#ifndef DATATYPES_H
#define DATATYPES_H

#include <time.h>    // Per time_t
#include <stdbool.h> // Per bool type
#include "config.h"  // Includiamo le costanti definite

// --- Strutture e Tipi relativi ai Soccorritori ---

/**
 * @brief Enumera gli stati possibili di un'istanza di soccorritore.
 *        (Definizione dal PDF)
 */
typedef enum {
    IDLE,
    EN_ROUTE_TO_SCENE,
    ON_SCENE,
    RETURNING_TO_BASE
} rescuer_status_t;

/**
 * @brief Definisce le caratteristiche di un *tipo* di soccorritore.
 *        (Definizione dal PDF)
 */
typedef struct {
    char *rescuer_type_name; // Nome identificativo del tipo (es. "Pompieri") - DA ALLOCARE DINAMICAMENTE
    int speed;               // Velocità in "caselle al secondo"
    int x;                   // Coordinata X della base
    int y;                   // Coordinata Y della base
} rescuer_type_t;

/**
 * @brief Rappresenta una singola unità di soccorso (istanza o "gemello digitale").
 *        (Definizione dal PDF)
 */
typedef struct {
    int id;
    int x;
    int y;
    rescuer_type_t *rescuer; // Puntatore al tipo di soccorritore
    rescuer_status_t status;
} rescuer_digital_twin_t;


// --- Strutture e Tipi relativ i alle Emergenze ---

/**
 * @brief Definisce una richiesta specifica di un tipo e numero di soccorritori
 *        per un certo tipo di emergenza.
 *        (Definizione dal PDF)
 */
typedef struct {
    rescuer_type_t *type; // Puntatore al *tipo* di soccorritore richiesto
    int required_count;   // Numero di unità di questo tipo necessarie
    int time_to_manage;   // Tempo (in secondi) che ogni unità impiegherà sull'emergenza
} rescuer_request_t;


/**
 * @brief Definisce le caratteristiche di un *tipo* di emergenza.
 *        (Definizione dal PDF)
 */
typedef struct {
    short priority;
    char *emergency_desc;           // Nome/descrizione del tipo di emergenza - DA ALLOCARE DINAMICAMENTE
    rescuer_request_t *rescuers;    // Array delle richieste di soccorritori (tipo, numero, tempo)
    int rescuers_req_number;        // Numero di elementi nell'array 'rescuers'
} emergency_type_t;

/**
 * @brief Enumera gli stati possibili di un'istanza di emergenza.
 *        (Definizione dal PDF)
 */
typedef enum {
    WAITING,
    ASSIGNED,
    IN_PROGRESS,
    PAUSED,
    COMPLETED,
    CANCELED,
    TIMEOUT
} emergency_status_t;

/**
 * @brief Rappresenta il messaggio grezzo ricevuto dalla coda di messaggi.
 *        (Definizione dal PDF, ma nome campo `emergency_name` invece di `emergency_name[EMERGENCY_NAME_LENGTH]`)
 *        Dato che `EMERGENCY_NAME_LENGTH` è definito, si presumeva un array. Se è char*, va allocato.
 *        Presumo che `emergency_name` nella richiesta sia un nome che verrà poi confrontato.
 *        Per coerenza con le altre stringhe `char*` e per permettere nomi di lunghezza variabile dal client,
 *        il client dovrebbe inviare una stringa null-terminated. Il server la leggerà.
 *        Per semplicità nel parsing del messaggio dalla coda, userò un buffer di dimensione fissa.
 */
typedef struct {
    char emergency_name[EMERGENCY_NAME_LENGTH]; // Nome del tipo di emergenza richiesto
    int x;
    int y;
    time_t timestamp;
} emergency_request_t; // Questa è la struttura del MESSAGGIO, non dell'entità interna.

/**
 * @brief Rappresenta un'istanza specifica di emergenza in corso di gestione.
 *        (Definizione dal PDF)
 */
typedef struct {
    emergency_type_t type; // NOTA PDF: qui è `emergency_type_t type;` (valore) non puntatore.
                           // Questo è insolito e implica una copia. Confermerò se si intende un puntatore.
                           // Se è per valore, `type.emergency_desc` e `type.rescuers` devono essere gestiti attentamente.
                           // Per ora lo lascio come da PDF, ma sospetto sia un errore e dovrebbe essere `emergency_type_t *type;`
                           // Se è per valore, e `emergency_desc` è `char*`, allora `emergency_t` deve duplicare quella stringa.
                           // AGGIORNAMENTO: È molto più probabile che sia `emergency_type_t *type;`. Modifico in tal senso.
                           // Se fosse per valore, ogni `emergency_t` conterrebbe una copia completa dell'emergency_type,
                           // inclusi l'array `rescuers` e la stringa `emergency_desc`, che è inefficiente e complesso da gestire.
    emergency_type_t *type;

    emergency_status_t status;
    int x;
    int y;
    time_t time; // Timestamp di quando l'emergenza è stata registrata/processata
    int rescuer_count; // Numero di soccorritori assegnati
    rescuer_digital_twin_t **rescuers_dt; // Array di PUNTATORI ai soccorritori digitali assegnati.
                                          // PDF dice `rescuer_digital_twin_t* rescuers_dt;`
                                          // che solitamente indica un puntatore a UN digital twin, o al PRIMO di un array.
                                          // Ma dato `rescuer_count`, si intende una collezione.
                                          // `rescuer_digital_twin_t** rescuers_dt;` è più chiaro per un array di puntatori.
                                          // o `rescuer_digital_twin_t* rescuers_dt;` e si alloca un array di `rescuer_digital_twin_t`.
                                          // Sceglierò `rescuer_digital_twin_t** rescuers_dt;` per chiarezza, come array di puntatori.
                                          // Questo significa che `rescuers_dt` è un puntatore a un array di `rescuer_digital_twin_t*`.
} emergency_t;


// --- Struttura per l'Ambiente (come da esempio file, non da PDF) ---
/**
 * @brief Contiene le informazioni globali sull'ambiente operativo.
 */
typedef struct {
    char queue_name[MAX_LINE_LENGTH]; // Nome della coda di messaggi POSIX
    int grid_height;                  // Altezza della griglia
    int grid_width;                   // Larghezza della griglia
} environment_t;


// --- Struttura di supporto per contenere tutta la configurazione parsata ---
// Questa è una struttura di convenienza, non definita nel PDF.
typedef struct {
    environment_t env_config;

    rescuer_type_t *rescuer_types_array; // Array dei tipi di soccorritori
    int num_rescuer_types;               // Numero di tipi di soccorritori nell'array

    emergency_type_t *emergency_types_array; // Array dei tipi di emergenze
    int num_emergency_types;                 // Numero di tipi di emergenze nell'array

    // Aggiungiamo un campo per il numero totale di soccorritori digital twin da creare,
    // perché non è più immagazzinato direttamente in rescuer_type_t.
    int total_digital_twins_to_create;

} system_config_t;


#endif // DATATYPES_H