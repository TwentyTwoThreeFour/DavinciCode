#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define SQKEY (key_t)60070	// server basic key
#define CQKEY (key_t)60074	// client basic key
#define QPERM 0666
#define MAXOBN 32
#define MAXINDEX 4
#define MAXSIGN 10
#define BLOCKNUM 26

struct q_entry {
	long mtype;
	int message;
	int pid;
	int blocks[BLOCKNUM];
	int user1[BLOCKNUM];
	int user2[BLOCKNUM];
	int userstatus;
};
