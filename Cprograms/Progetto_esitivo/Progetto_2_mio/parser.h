#ifndef PARSER_H
#define PARSER_H

#include"data_types.h"
#include<stdio.h>

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
 * @param config Puntatore alla system_config_t dove verranno memorizzati
 *               l'array dei tipi di soccorritori, il loro numero, e il
 *               numero totale di digital twin da creare.
 *               La funzione allocherà memoria per l'array rescuer_types_array.
 * @return true se il parsing ha avuto successo, false altrimenti.
 *         In caso di fallimento, la memoria allocata parzialmente viene liberata.
 */
bool parse_rescuers(const char *filename, system_config_t *config);

/**
 * @brief Parsa il file di configurazione dei tipi di emergenza.
 *
 * @param filename Il percorso del file emergency_types.conf.
 * @param config Puntatore alla system_config_t. È necessario avere già parsato
 *               i tipi di soccorritori (config->rescuer_types_array) per
 *               collegare le richieste ai tipi esistenti.
 *               La funzione allocherà memoria per l'array emergency_types_array.
 * @return true se il parsing ha avuto successo, false altrimenti.
 *         In caso di fallimento, la memoria allocata parzialmente viene liberata.
 */
bool parse_emergency_types(const char *filename, system_config_t *config);

/**
 * @brief Funzione helper per liberare la memoria allocata per system_config_t.
 *        Questo include i nomi allocati dinamicamente e gli array di tipi.
 *
 * @param config Puntatore alla configurazione da liberare.
 */
void free_system_config(system_config_t *config);   //messa in parse_rescuers

#endif