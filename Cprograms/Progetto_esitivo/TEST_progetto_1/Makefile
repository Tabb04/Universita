# Makefile Semplificato per Sistema di Gestione Emergenze

# --- Compilatore e Flags ---
CC = gcc
# CFLAGS per tutti i file: warning, standard C11, debug symbols, ottimizzazione
CFLAGS = -Wall -Wextra -std=c11 -g -O2
# LDFLAGS per il linking: libreria realtime e pthreads
LDFLAGS = -lrt -pthread

# --- Nomi degli Eseguibili ---
SERVER_EXEC = emergency_server
CLIENT_EXEC = client
INTERACTIVE_CLIENT_EXEC = client_interactive

# --- File Sorgente ---
# Aggiungi qui tutti i tuoi file .c che compongono il server
SERVER_SRCS = main.c logger.c parse_env.c parse_rescuers.c parse_emergency_types.c
# Aggiungi altri file .c del server se ne hai

# File .c per i Client
CLIENT_SRCS = client.c
INTERACTIVE_CLIENT_SRCS = client_interactive.c

# --- Oggetti (.o) ---
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
INTERACTIVE_CLIENT_OBJS = $(INTERACTIVE_CLIENT_SRCS:.c=.o)

# --- File Header (Lista approssimativa, mantienila aggiornata) ---
# Una gestione più avanzata delle dipendenze dagli header è possibile ma più complessa.
HDRS = data_types.h config1.h logger.h parser.h Syscalls_3_progetto.h

# --- Regole Principali ---
# 'all' è la regola di default, compila server e entrambi i client
all: $(SERVER_EXEC) $(CLIENT_EXEC) $(INTERACTIVE_CLIENT_EXEC)

# Regola per compilare il SERVER
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_EXEC) $(LDFLAGS)
	@echo "Server '$(SERVER_EXEC)' compilato con successo."

# Regola per compilare il CLIENT (batch)
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_EXEC) $(LDFLAGS)
	@echo "Client (batch) '$(CLIENT_EXEC)' compilato con successo."

# Regola per compilare il CLIENT INTERATTIVO
$(INTERACTIVE_CLIENT_EXEC): $(INTERACTIVE_CLIENT_OBJS)
	$(CC) $(CFLAGS) $(INTERACTIVE_CLIENT_OBJS) -o $(INTERACTIVE_CLIENT_EXEC) $(LDFLAGS)
	@echo "Client (interattivo) '$(INTERACTIVE_CLIENT_EXEC)' compilato con successo."

# --- Regola Generica per compilare i file .c in .o ---
# Dipendenza da $(HDRS): se un header cambia, forza la ricompilazione degli oggetti.
# Per dipendenze precise, servirebbe la generazione automatica delle dipendenze.
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Regola di Pulizia ---
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(INTERACTIVE_CLIENT_OBJS) \
	      $(SERVER_EXEC) $(CLIENT_EXEC) $(INTERACTIVE_CLIENT_EXEC) \
	      *.log core gmon.out
	@echo "File oggetto, eseguibili, log, core dump e gmon.out rimossi."

# Per rendere le regole 'all' e 'clean' non considerate come file
.PHONY: all clean