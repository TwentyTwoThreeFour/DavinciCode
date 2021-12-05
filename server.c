#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "q.h"

int serve(void);
int warn(char *s);
int init_queue(void);
int proc_obj(struct q_entry *rcv, int *member, int *memcnt);
char *substr(int s, int e, char *str);
int enter(char *msg, int signal);

int main(void) {
	int pid;

	switch (pid = fork()) {
		case 0:
			serve();
			break;
		case -1:
			warn("fork to start server failed");
			break;
		default:
			printf("server process pid is %d\n", pid);
	}
	exit(pid != -1 ? 0 : 1);
}

int warn(char *s) {
	fprintf(stderr, "warning: %s\n", s);
}

int init_queue(void) {
	int queue_id;

	if ((queue_id = msgget(QKEY, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}

	return (queue_id);
}

int serve(void) {
	int mlen, r_qid;
	struct q_entry r_entry;
	int member[4];
	int memcnt = 0;

	if ((r_qid = init_queue()) == -1) {
		return (-1);
	}

	for (;;) {
		if ((mlen = msgrcv(r_qid, &r_entry, MAXOBN, (-1 * MAXPRIOR), MSG_NOERROR)) == -1) {
			perror("msgrcv failed");
			return (-1);
		}
		else {
			r_entry.mtext[mlen] = '\0';

			proc_obj(&r_entry, member, &memcnt);
		}
	}
}

int proc_obj(struct q_entry *rcv, int *member, int *memcnt) {
	/* receive signal
	 * 1: 클라이언트 진입 
	 * 2: 블록 색 선정
	 * 3: 조커 위치 선정(필요 시) */
	if (rcv->mtype == 1) {
		member[*memcnt] = atoi(rcv->mtext);
		char *msg;

		printf("system: %d entered\n", member[*memcnt]);
		sprintf(msg, "%d", member[*memcnt]);
		if (enter(msg, 1) < 0) {
			warn("send failure");
		}
		(*memcnt)++;

		printf("system: %d members\n", *memcnt);
		sprintf(msg, "%d", *memcnt);
		if (enter(msg, 2) < 0) {
			warn("send failure");
		}
	}
	else if (rcv->mtype == 2) {
		printf("mtype: 2\n");
	}

	printf("priority: %ld name: %s\n", rcv->mtype, rcv->mtext);
}

char *substr(int s, int e, char *str) {
	char *new = (char *)malloc(sizeof(char)*(e-s+2));
	strncpy(new, str+s, e-s+1);
}

int enter(char *msg, int signal) {
	int len, s_qid;
	struct q_entry s_entry;

	if ((len = strlen(msg)) > MAXOBN) {
		warn("string too long");
		return (-1);
	}

	if (signal > MAXPRIOR || signal < 0) {
		warn("invalid signal");
		return (-1);
	}

	if ((s_qid = init_queue()) == -1) {
		return (-1);
	}

	s_entry.mtype = (long)signal;
	strncpy(s_entry.mtext, msg, MAXOBN);

	if (msgsnd(s_qid, &s_entry, strlen(msg), 0) == -1) {
		perror("msgsnd failed");
		return (-1);
	}
	else {
		return (0);
	}
}