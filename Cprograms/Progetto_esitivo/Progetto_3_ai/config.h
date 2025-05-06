#ifndef CONFIG_H
#define CONFIG_H

// Lunghezza massima per i nomi delle emergenze (come da specifica)
#define EMERGENCY_NAME_LENGTH 64

// Lunghezza massima per i nomi dei tipi di soccorritori (definiamo un limite ragionevole)
#define RESCUER_NAME_MAX_LEN 64 // Anche se nel PDF il nome è char*, usiamo una dimensione fissa per semplicità nei parser
                                 // e perché rescuer_type_name in rescuer_type_t nel PDF usa char*
                                 // Questo va chiarito: se rescuer_type_name è char*, va allocato dinamicamente.
                                 // Per ora, presumo che nel parser possiamo usare un buffer e poi decidere.
                                 // Nelle struct originali è char*, il che implica allocazione dinamica.

// Lunghezza massima di una riga letta dai file di configurazione
#define MAX_LINE_LENGTH 1024

// Valori per le priorità delle emergenze (come da specifica)
#define PRIORITY_LOW    0
#define PRIORITY_MEDIUM 1
#define PRIORITY_HIGH   2

// Costanti per i timeout delle priorità in secondi (come da specifica)
#define TIMEOUT_PRIORITY_1 30 // Secondi per priorità 1 (MEDIUM)
#define TIMEOUT_PRIORITY_2 10 // Secondi per priorità 2 (HIGH)
// Priorità 0 (LOW) non ha timeout esplicito

#endif // CONFIG_H