CC = clang
CFLAGS = -Wall -pthread

all: server client

debug: clean all

server: src/server.c src/list.c
	@$(CC) $(CFLAGS) src/server.c src/list.c -o server

client: src/client.c
	@$(CC) $(CFLAGS) src/client.c -o client

.PHONY: clean
clean:
	@rm -f server client