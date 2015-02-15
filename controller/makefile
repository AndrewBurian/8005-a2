# makefile
CC=gcc
CFLAGS=-c -g -Wall
LDFLAGS=
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=controller

SOURCES=controller.c discover.c

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
