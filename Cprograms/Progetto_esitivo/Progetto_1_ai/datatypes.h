#ifndef DATATYPES_H
#define DATATYPES_H

#include <time.h>    // Per time_t
#include <stdbool.h> // Per bool type
#include "config.h"  // Includiamo le costanti definite

// --- Strutture e Tipi relativi ai Soccorritori ---

/**
 * @brief Enumera gli stati possibili di un'istanza di soccorritore (digital twin).
 */
typedef enum {
    IDLE,               // Libero, in attesa alla base
    EN_ROUTE_TO_SCENE,  // In viaggio verso la scena dell'emergenza
    ON_SCENE,           // Arrivato sulla scena dell'emergenza, operativo
    RETURNING_TO_BASE   // In viaggio di ritorno verso la base
} rescuer_status_t;

/**
 * @brief Definisce le caratteristiche di un *tipo* di soccorritore.
 *        Es: "Pompieri", "Ambulanza".
 */
typedef struct rescuer_type {
    char name[RESCUER_NAME_MAX_LEN]; // Nome identificativo del tipo (es. "Pompieri")
    int speed;                       // Velocità in "caselle al secondo"
    int base_x;                      // Coordinata X della base
    int base_y;                      // Coordinata Y della base
    // Aggiungiamo un contatore per sapere quante istanze di questo tipo ci sono
    int total_count;                 // Numero totale di mezzi di questo tipo disponibili
    struct rescuer_type* next;       // Puntatore per creare una lista linkata (opzionale, ma utile per il parsing)
} rescuer_type_t;

/**
 * @brief Rappresenta una singola unità di soccorso (istanza o "gemello digitale").
 */
typedef struct rescuer_digital_twin {
    int id;                         // ID univoco dell'istanza del soccorritore
    rescuer_type_t *type;           // Puntatore al tipo di soccorritore (definisce capacità, velocità, etc.)
    int current_x;                  // Posizione X attuale
    int current_y;                  // Posizione Y attuale
    rescuer_status_t status;        // Stato attuale del soccorritore (IDLE, ON_SCENE, etc.)
    int assigned_emergency_id;      // ID dell'emergenza a cui è assegnato (-1 se IDLE o RETURNING)
    // Potremmo aggiungere time_t last_status_change; per logging/tracking più fine
    struct rescuer_digital_twin* next; // Puntatore per lista linkata (opzionale)
} rescuer_digital_twin_t;


// --- Strutture e Tipi relativi alle Emergenze ---

/**
 * @brief Definisce una richiesta specifica di un tipo e numero di soccorritori
 *        per un certo tipo di emergenza.
 *        Es: "Per un Incendio servono 2 Pompieri per 8 secondi".
 */
typedef struct {
    rescuer_type_t *type; // Puntatore al *tipo* di soccorritore richiesto (es. Pompieri)
    int required_count;   // Numero di unità di questo tipo necessarie
    int time_to_manage;   // Tempo (in secondi) che ogni unità impiegherà sull'emergenza
} rescuer_request_t;

/**
 * @brief Definisce le caratteristiche di un *tipo* di emergenza.
 *        Es: "Incendio", "Allagamento".
 */
typedef struct emergency_type {
    char emergency_desc[EMERGENCY_NAME_LENGTH]; // Nome/descrizione del tipo di emergenza (es. "Incendio")
    short priority;                           // Priorità (0=Bassa, 1=Media, 2=Alta)
    rescuer_request_t *rescuers;              // Array delle richieste di soccorritori (tipo, numero, tempo)
    int rescuers_req_number;                  // Numero di elementi nell'array 'rescuers'
    struct emergency_type* next;              // Puntatore per lista linkata (opzionale)
} emergency_type_t;

/**
 * @brief Enumera gli stati possibili di un'istanza di emergenza.
 */
typedef enum {
    WAITING,        // Ricevuta, in attesa di assegnazione soccorritori
    ASSIGNED,       // Soccorritori necessari identificati e assegnati (ma non ancora tutti in viaggio/arrivati)
    IN_PROGRESS,    // Tutti i soccorritori assegnati sono arrivati sulla scena
    PAUSED,         // Stato non descritto esplicitamente, ma presente nell'enum originale (potrebbe servire?)
    COMPLETED,      // Emergenza gestita con successo
    CANCELED,       // Emergenza annullata (motivo da loggare?)
    TIMEOUT         // Impossibile gestire l'emergenza entro il tempo limite (priorità 1 o 2)
} emergency_status_t;

/**
 * @brief Rappresenta il messaggio grezzo ricevuto dalla coda di messaggi.
 */
typedef struct {
    char emergency_name[EMERGENCY_NAME_LENGTH]; // Nome del tipo di emergenza richiesto
    int x;                                      // Coordinata X dell'emergenza
    int y;                                      // Coordinata Y dell'emergenza
    time_t timestamp;                           // Timestamp di quando l'emergenza è stata registrata (dal client)
    // Potremmo aggiungere un ID univoco generato alla ricezione qui o nell'emergency_t
} emergency_request_t;

/**
 * @brief Rappresenta un'istanza specifica di emergenza in corso di gestione.
 */
typedef struct emergency {
    int id;                              // ID univoco assegnato all'emergenza dal sistema
    emergency_type_t *type;              // Puntatore al tipo di emergenza corrispondente
    emergency_status_t status;           // Stato attuale dell'emergenza (WAITING, ASSIGNED, etc.)
    int x;                               // Coordinata X della scena dell'emergenza
    int y;                               // Coordinata Y della scena dell'emergenza
    time_t received_time;                // Timestamp di quando il sistema ha ricevuto/validato la richiesta
    time_t deadline;                     // Timestamp entro cui deve diventare IN_PROGRESS (se priorità > 0)
    // Gestione Soccorritori Assegnati:
    int assigned_rescuers_count;         // Numero di soccorritori *attualmente* assegnati a questa emergenza
    rescuer_digital_twin_t **rescuers_dt; // Array di puntatori ai digital twin assegnati
                                         // Nota: la dimensione di questo array dovrebbe essere la somma
                                         // di tutti i required_count nel corrispondente emergency_type_t
    int total_rescuers_needed;           // Numero totale di soccorritori necessari (somma dei required_count)
    struct emergency* next;              // Puntatore per lista linkata (opzionale, per code interne)
} emergency_t;


// --- Struttura per l'Ambiente ---

/**
 * @brief Contiene le informazioni globali sull'ambiente operativo.
 */
typedef struct {
    char queue_name[MAX_LINE_LENGTH]; // Nome della coda di messaggi POSIX
    int grid_height;                  // Altezza della griglia
    int grid_width;                   // Larghezza della griglia
} environment_t;


// --- Struttura per contenere tutta la configurazione parsata ---
// Utile per passare i dati letti dai file alle funzioni di inizializzazione
typedef struct {
    environment_t env_config;
    rescuer_type_t* rescuer_types; // Testa della lista linkata dei tipi di soccorritori
    int total_rescuers;            // Numero totale di digital twin da creare (somma dei num per ogni tipo)
    emergency_type_t* emergency_types; // Testa della lista linkata dei tipi di emergenze
} system_config_t;


#endif // DATATYPES_H