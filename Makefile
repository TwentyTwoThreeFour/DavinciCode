CC=gcc

all: msg msgServer shm shmServer pipe pipeServer

msg: msg.c
	$(CC) -o $@ $< -lpthread

msgServer: msgServer.c
	$(CC) -o $@ $< -lpthread
	
shm: shm.c
	$(CC) -o $@ $< -lpthread

shmServer: shmServer.c
	$(CC) -o $@ $< -lpthread

pipe: pipe.c
	$(CC) -o $@ $< -lpthread

pipeServer: pipeServer.c
	$(CC) -o $@ $< -lpthread
