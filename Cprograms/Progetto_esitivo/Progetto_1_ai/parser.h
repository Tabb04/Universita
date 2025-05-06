#ifndef PARSER_H
#define PARSER_H

#include "datatypes.h" // Include le definizioni delle strutture dati
#include <stdio.h>    // Necessario per FILE*

/**
 * @brief Parsa il file di configurazione dell'ambiente.
 *
 * @param filename Il percorso del file env.conf.
 * @param env_config Puntatore alla struttura environment_t da popolare.
 * @return true se il parsing ha avuto successo, false altrimenti.
 */
bool parse_environment(const char *filename, environment_t *env_config);

/**
 * @brief Parsa il file di configurazione dei soccorritori.
 *
 * @param filename Il percorso del file rescuers.conf.
 * @param rescuer_type_list Doppia indirezione al puntatore alla testa della lista
 *                          linkata dei tipi di soccorritori da creare.
 *                          La funzione allocherà memoria per la lista.
 * @param total_rescuers_count Puntatore a un intero dove verrà scritto il numero
 *                             totale di istanze (digital twin) da creare.
 * @return true se il parsing ha avuto successo, false altrimenti.
 *         In caso di fallimento, la memoria allocata parzialmente viene liberata.
 */
bool parse_rescuers(const char *filename, rescuer_type_t **rescuer_type_list, int *total_rescuers_count);

/**
 * @brief Parsa il file di configurazione dei tipi di emergenza.
 *
 * @param filename Il percorso del file emergency_types.conf.
 * @param available_rescuers La lista linkata dei tipi di soccorritori già parsata
 *                           (necessaria per collegare le richieste ai tipi esistenti).
 * @param emergency_type_list Doppia indirezione al puntatore alla testa della lista
 *                            linkata dei tipi di emergenze da creare.
 *                            La funzione allocherà memoria per la lista.
 * @return true se il parsing ha avuto successo, false altrimenti.
 *         In caso di fallimento, la memoria allocata parzialmente viene liberata.
 */
bool parse_emergency_types(const char *filename, const rescuer_type_t *available_rescuers, emergency_type_t **emergency_type_list);

/**
 * @brief Funzione helper per liberare la memoria allocata per la lista dei tipi di soccorritori.
 *
 * @param head Puntatore alla testa della lista da liberare.
 */
void free_rescuer_type_list(rescuer_type_t *head);

/**
 * @brief Funzione helper per liberare la memoria allocata per la lista dei tipi di emergenze.
 *
 * @param head Puntatore alla testa della lista da liberare.
 */
void free_emergency_type_list(emergency_type_t *head);


#endif // PARSER_H