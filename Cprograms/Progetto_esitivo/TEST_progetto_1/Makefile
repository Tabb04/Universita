CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -g
LIBS = -lrt -lpthread -lm
#senza lm non funziona per alcune librerie per qualche motivo

SERVER_NAME = emergency_server
CLIENT_NAME = client

SERVER_SRCS = main.c logger.c parse_env.c parse_rescuers.c parse_emergency_types.c 
CLIENT_SRCS = client.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

HDRS = data_types.h config1.h logger.h parser.h Syscalls_3_progetto.h

all: $(SERVER_NAME) $(CLIENT_NAME)

%.o: %.c $(HDRS)
	$(CC) -c $(CFLAGS) $< -o $@

$(SERVER_NAME): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(CLIENT_NAME): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(SERVER_NAME) $(CLIENT_NAME) \
	      $(SERVER_OBJS) $(CLIENT_OBJS) \
	      *.log core

run: $(SERVER_NAME)
	./$(SERVER_NAME)

.PHONY: all clean run