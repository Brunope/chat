CC = gcc
CFLAGS = -g -std=gnu99 -Wall -pthread -lncurses
CLIENT_FILES = src/client/client.c src/client/mlist.c src/client/talker.c
SERVER_FILES = src/server/server.c
UTIL_FILES = src/utils/utils.c

all: client-server

client-server: client server

client: clean-client
	$(CC) $(CFLAGS) -o client $(CLIENT_FILES) $(UTIL_FILES)

server: clean-server
	$(CC) $(CFLAGS) -o server $(SERVER_FILES) $(UTIL_FILES)

clean: clean-client clean-server

clean-client:
	rm -rf src/client/client src/client/client.dSYM

clean-server:
	rm -rf src/server/server src/server/server.dSYM
