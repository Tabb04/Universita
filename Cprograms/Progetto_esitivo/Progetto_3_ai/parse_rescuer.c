#include "parser.h"
#include "datatypes.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

// Funzione trim_whitespace (può essere duplicata o messa in un file utils.c comune)
static void trim_whitespace_res(char *str) { // rinominata per evitare conflitti se non è static
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}


// Helper per il parsing di rescuers: prima contiamo gli elementi, poi allochiamo
bool parse_rescuers(const char *filename, system_config_t *config) {
    if (!filename || !config) {
        fprintf(stderr, "Errore: Argomenti nulli passati a parse_rescuers.\n");
        return false;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file rescuers");
        fprintf(stderr, "Impossibile aprire il file di configurazione soccorritori: %s\n", filename);
        return false;
    }

    char line[MAX_LINE_LENGTH];
    int line_num = 0;
    int type_count = 0;

    // Primo Passaggio: Contare il numero di tipi di soccorritori validi
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line = line;
        trim_whitespace_res(trimmed_line);
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }
        // Fai un controllo base del formato per contare solo righe potenzialmente valide
        if (sscanf(trimmed_line, " [%*[^]]] [%*d] [%*d] [%*d;%*d]") == 0) { // %* non assegna
             // Questo sscanf è un po' rozzo per il conteggio, ma meglio di niente
             // Potrebbe contare righe parzialmente errate. Una validazione completa è nel secondo pass.
        }
        type_count++;
    }

    if (type_count == 0) {
        fprintf(stdout, "Attenzione: Nessun tipo di soccorritore trovato nel file %s.\n", filename);
        config->num_rescuer_types = 0;
        config->rescuer_types_array = NULL;
        config->total_digital_twins_to_create = 0;
        fclose(file);
        return true; // Non un errore fatale se il file è vuoto ma valido
    }

    // Alloca l'array per i tipi di soccorritori
    config->rescuer_types_array = (rescuer_type_t *)malloc(type_count * sizeof(rescuer_type_t));
    if (!config->rescuer_types_array) {
        perror("Errore allocazione memoria per rescuer_types_array");
        fclose(file);
        return false;
    }
    config->num_rescuer_types = 0; // Sarà incrementato nel secondo passaggio
    config->total_digital_twins_to_create = 0;

    // Riporta il file pointer all'inizio
    rewind(file);
    line_num = 0;
    int current_type_idx = 0;

    // Secondo Passaggio: Leggere e popolare i dati
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        line[strcspn(line, "\n")] = 0;
        char *trimmed_line = line;
        trim_whitespace_res(trimmed_line);
        if (*trimmed_line == '\0' || *trimmed_line == '#') {
            continue;
        }

        char name_buffer[RESCUER_NAME_MAX_LEN];
        int num_instances, speed, base_x, base_y;

        int matched = sscanf(trimmed_line, " [%[^]]] [%d] [%d] [%d;%d]",
                             name_buffer, &num_instances, &speed, &base_x, &base_y);

        if (matched != 5) {
            fprintf(stderr, "Errore parsing rescuers: Formato linea non valido alla riga %d: '%s'\n", line_num, trimmed_line);
            // Liberare memoria allocata finora per i nomi e l'array stesso
            for (int i = 0; i < current_type_idx; ++i) {
                free(config->rescuer_types_array[i].rescuer_type_name);
            }
            free(config->rescuer_types_array);
            config->rescuer_types_array = NULL;
            config->num_rescuer_types = 0;
            fclose(file);
            return false;
        }

        if (num_instances <= 0 || speed <= 0 || base_x < 0 || base_y < 0 || strlen(name_buffer) == 0) {
            fprintf(stderr, "Errore parsing rescuers: Valori non validi (num=%d, speed=%d, base_x=%d, base_y=%d, nome='%s') alla riga %d\n",
                    num_instances, speed, base_x, base_y, name_buffer, line_num);
            // Cleanup come sopra
            for (int i = 0; i < current_type_idx; ++i) free(config->rescuer_types_array[i].rescuer_type_name);
            free(config->rescuer_types_array);
            config->rescuer_types_array = NULL; config->num_rescuer_types = 0;
            fclose(file);
            return false;
        }

        rescuer_type_t *current_rt = &config->rescuer_types_array[current_type_idx];
        
        current_rt->rescuer_type_name = strdup(name_buffer); // Alloca e copia nome
        if (!current_rt->rescuer_type_name) {
            perror("Errore allocazione memoria per rescuer_type_name");
            for (int i = 0; i < current_type_idx; ++i) free(config->rescuer_types_array[i].rescuer_type_name);
            // Non c'è bisogno di liberare current_rt->rescuer_type_name perché è fallito strdup
            free(config->rescuer_types_array);
            config->rescuer_types_array = NULL; config->num_rescuer_types = 0;
            fclose(file);
            return false;
        }
        current_rt->speed = speed;
        current_rt->x = base_x;
        current_rt->y = base_y;
        // Il numero di istanze (num_instances) non è nella struct rescuer_type_t
        // Lo usiamo per calcolare il totale
        config->total_digital_twins_to_create += num_instances;
        
        current_type_idx++;
        config->num_rescuer_types = current_type_idx; // Aggiorna il conteggio effettivo

        printf("  Letto tipo soccorritore: %s (Speed: %d, Base: %d;%d) - Istanze da creare per questo tipo: %d\n",
               current_rt->rescuer_type_name, current_rt->speed, current_rt->x, current_rt->y, num_instances);

    } // fine while fgets

    fclose(file);

    if (config->num_rescuer_types != type_count && type_count > 0) {
        // Questo indica un errore nel conteggio del primo passaggio o un errore nel secondo
        // che non ha portato a un return false immediato ma ha interrotto il loop.
        // Per sicurezza, liberiamo tutto.
        fprintf(stderr, "Discordanza nel conteggio dei tipi di soccorritori. Pulizia.\n");
        for (int i = 0; i < config->num_rescuer_types; ++i) { // num_rescuer_types è l'idx corrente
            free(config->rescuer_types_array[i].rescuer_type_name);
        }
        free(config->rescuer_types_array);
        config->rescuer_types_array = NULL;
        config->num_rescuer_types = 0;
        config->total_digital_twins_to_create = 0;
        return false;
    }


    printf("Parsing soccorritori completato. Tipi letti: %d. Totale gemelli digitali da creare: %d\n",
           config->num_rescuer_types, config->total_digital_twins_to_create);
    return true;
}