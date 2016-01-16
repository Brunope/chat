CC = gcc
CFLAGS = -g -std=gnu99 -Wall -pthread -lncurses
CLIENT_FILES = client.c utils.c mlist.c talker.c
SERVER_FILES = server.c utils.c

all: client-server

client-server: client server

client: clean-client
	$(CC) $(CFLAGS) -o client $(CLIENT_FILES)

server: clean-server
	$(CC) $(CFLAGS) -o server $(SERVER_FILES)

clean: clean-client clean-server

clean-client:
	rm -f client

clean-server:
	rm -f server
