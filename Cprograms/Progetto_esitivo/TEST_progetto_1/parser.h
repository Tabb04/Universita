#ifndef PARSER_H
#define PARSER_H

#include"data_types.h"
#include<stdio.h>

bool parse_environment(const char *filename, environment_t *env_config);

bool parse_rescuers(const char *filename, system_config_t *config);

bool parse_emergency_types(const char *filename, system_config_t *config);

void free_system_config(system_config_t *config);   //messa in parse_rescuers

#endif