CCFLAGS=-Wall -Wextra

all: server

server: server.c
	$(CC) $(CCFLAGS) server.c -o server

clean:
	rm -f server
