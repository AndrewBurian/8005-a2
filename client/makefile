# makefile
CC=gcc
CFLAGS=-c -g -Wall
LDFLAGS=
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=client.exe

SOURCES=client.c discover.c preptest.c runtest.c

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
