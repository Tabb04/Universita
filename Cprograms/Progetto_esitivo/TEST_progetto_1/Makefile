# Makefile per il Sistema di Gestione Emergenze

# --- Compilatore e Flags ---
CC = gcc
# CFLAGS per tutti i file: warning, standard C11, debug symbols, ottimizzazione
CFLAGS = -Wall -Wextra -std=c11 -g -O2
# LDFLAGS per il linking: libreria realtime e pthreads
LDFLAGS = -lrt -pthread

# --- Nomi degli Eseguibili ---
SERVER_EXEC = emergency_server
CLIENT_EXEC = client

# --- File Sorgente ---
# Lista dei file .c per il SERVER (escludi client.c)
# Aggiungi qui tutti i tuoi file .c che compongono il server
SERVER_SRCS = main.c logger.c parse_env.c parse_rescuers.c parse_emergency_types.c
# Aggiungi altri file .c del server se ne hai (es. utils.c, etc.)

# File .c per il CLIENT
CLIENT_SRCS = client.c

# --- Oggetti (.o) ---
# Genera automaticamente la lista degli oggetti dai sorgenti del server
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
# Genera automaticamente la lista degli oggetti dai sorgenti del client
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# --- File Header ---
# Lista degli header. Se un header cambia, forza la ricompilazione degli oggetti dipendenti.
# Questa è una lista approssimativa, potresti doverla adattare.
# Un approccio più avanzato userebbe 'gcc -MMD -MP' per generare dipendenze automaticamente.
HDRS = data_types.h config1.h logger.h parser.h Syscalls_3_progetto.h

# --- Regole Principali ---
# 'all' è la regola di default, compila sia server che client
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Regola per compilare il SERVER
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_EXEC) $(LDFLAGS)
	@echo "Server '$(SERVER_EXEC)' compilato con successo."

# Regola per compilare il CLIENT
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_EXEC) $(LDFLAGS)
	@echo "Client '$(CLIENT_EXEC)' compilato con successo."

# --- Regole Generiche per compilare i file .c in .o ---
# Questa regola dice a make come trasformare un .c in un .o
# Il $< si riferisce al primo prerequisito (il file .c)
# Il $@ si riferisce al target (il file .o)
# La dipendenza da $(HDRS) non è perfetta ma aiuta. Per dipendenze precise, vedi sotto.
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Regole di Pulizia ---
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC) *.log
	@echo "File oggetto, eseguibili e log rimossi."

# --- Regola per Eseguire il Server (Esempio) ---
# Questo è solo un esempio, potresti non volerlo direttamente nel Makefile
# o potresti volerlo fare in modo diverso (es. con uno script).
run_server: $(SERVER_EXEC)
	@echo "Avvio del server '$(SERVER_EXEC)'..."
	./$(SERVER_EXEC)

# --- Regola per Eseguire il Client (Esempio con argomenti fissi) ---
# Questo è solo un esempio. È più probabile che tu esegua il client manualmente.
run_client_test: $(CLIENT_EXEC)
	@echo "Invio richiesta di test con client '$(CLIENT_EXEC)'..."
	./$(CLIENT_EXEC) Incendio 10 20 0

# Per rendere le regole 'all', 'clean', 'run_server', 'run_client_test' non considerate come file
.PHONY: all clean run_server run_client_test

# --- Dipendenze Avanzate (Opzionale ma Raccomandato per Progetti più Grandi) ---
# Per una gestione più precisa delle dipendenze dagli header, puoi usare:
# CFLAGS += -MMD -MP
# E poi includere i file .d generati:
# -include $(SERVER_OBJS:.o=.d)
# -include $(CLIENT_OBJS:.o=.d)
# Questo richiede che la regola %.o: %.c generi anche il file .d.
# La regola diventerebbe:
# %.o: %.c
#	  $(CC) $(CFLAGS) -c $< -o $@
# Questa parte è un po' più avanzata. Per il tuo progetto attuale, la lista $(HDRS)
# potrebbe essere sufficiente se la mantieni aggiornata.