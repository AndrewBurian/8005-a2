# makefile
CC=gcc
CFLAGS=-c -g -Wall
LDFLAGS=
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=c10k.exe

SOURCES=main.c epoll.c poll.c select.c common.c

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
