CC=gcc
CFLAGS=-pthread -I inc/

LDFLAGS=

SOURCES_PATH=src
SOURCES_EXTENSION=c
SOURCES=$(shell find $(SOURCES_PATH) -name '*.$(SOURCES_EXTENSION)')

TARGET=udp2can


.PHONY: all clean

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -rf $(TARGET) *.o