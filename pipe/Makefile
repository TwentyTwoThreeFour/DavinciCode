CC=gcc

all: client server

client: message.c
	$(CC) -o $@ $< -lpthread

server: server.c
	$(CC) -o $@ $< -lpthread
