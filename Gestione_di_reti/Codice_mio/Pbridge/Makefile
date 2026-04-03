CC=gcc
CFLAGS=-Wall -Wextra -O2
LIBS=-lpcap
TARGET=pbridge
SRC=pbridge_1.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)