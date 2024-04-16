CC = gcc
CFLAGS = -g -Wall
OBJS = server.o
	
BINARIES = server

all: $(BINARIES)

server: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(OBJS) $(BINARIES)