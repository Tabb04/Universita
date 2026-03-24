CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lpcap
TARGET = packet_send
SRCS = packet_send.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)

# Show available packet types
list:
	@$(TARGET) -h

.PHONY: all clean list
