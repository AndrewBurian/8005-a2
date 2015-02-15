# makefile
CC=gcc
CFLAGS=-c -g -Wall
LDFLAGS=
LIBS=-lpthread
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=c10k.exe

SOURCES=server.c epoll.c poll.c select.c common.c

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
