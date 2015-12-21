CC = gcc
CFLAGS = -g -std=gnu99 -pthread
CLIENT_FILES = client.c utils.c
SERVER_FILES = server.c utils.c

all: client-server

client-server: client server

client: clean
	$(CC) $(CFLAGS) -o client $(CLIENT_FILES)

server: clean
	$(CC) $(CFLAGS) -o server $(SERVER_FILES)

clean:
	rm -f client server
