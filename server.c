#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "q.h"

#define THREAD_NUM 3

struct user {
	int hands[13];
};

int *serve(void *arg);
int *rcv_thread(void *arg);
int warn(char *s);
int init_squeue(void);
int init_cqueue(void);
int proc_obj(struct q_entry *rcv, int *member, int *memcnt);
char *substr(int s, int e, char *str);
int enter(char *msg, int signal);

int memcnt = 0;
int member[3];
char *msg;

int main(void) {
	int pid;
	pthread_t st_id[THREAD_NUM], ct_id[THREAD_NUM];
	int ckey[] = {CQKEY + 1, CQKEY + 2, CQKEY + 3};
	int skey[] = {SQKEY + 1, SQKEY + 2, SQKEY + 3};

	printf("Davinci Code Server Launched...\n");
	
	for (int i = 0; i < THREAD_NUM; i++) {
		if (pthread_create(&ct_id, NULL, rcv_thread, (void*)ckey[i]) != 0) {
			perror("thread create error");
			return -1;
		}

		if (pthread_create(&st_id, NULL, serve, (void*)skey[i]) != 0) {
			perror("thread create error");
			return -1;
		}
	}

	for (int i = 0; i < THREAD_NUM; i++) {
		if (pthread_join(ct_id[i], NULL) != 0) {
			perror("thread join error");
			return -1;
		}

		if (pthread_join(st_id[i], NULL) != 0) {
			perror("thread join error");
			return -1;
		}
	}

	exit(pid != -1 ? 0 : 1);
}

int *rcv_thread(void *arg) {
	int mlen, r_pid;
	struct q_entry r_entry;
	key_t ckey = *((key_t*)arg);

	if ((r_qid = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}

	for (;;) {
		if (mlen = msgrcv(r_qid, &r_entry, MAXOBN, (-1 * MAXPRIOR), MSG_NOERROR)) == -1) {
			perror("msgrcv failed");
		}
		else {
			r_entry.mtext[mlen] = '\0';
			
			proc_obj(&r_entry);
		}
	}
}

int *snd_thread(void *arg) {
	int len, s_qid;
	struct q_entry s_entry;
	key_t skey = *((key_t*)arg);
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

	if ((s_qid = init_squeue()) == -1) {
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

int proc_obj(struct q_entry *rcv) {
	/* receive signal
	 * 1: 클라이언트 진입 
	 * 2: 게임 시작 여부
	 * 3: 블록 색 선정
	 * 4: 조커 위치 선정(필요 시) */
	if (rcv->mtype == 1) {
		member[memcnt] = atoi(rcv->mtext);

		printf("system: %d entered\n", member[memcnt]);
		if (enter(rcv->mtext, 1) < 0) {
			warn("send failure");
		}
		(*memcnt)++;

		printf("system: %d members\n", *memcnt);
		sprintf(tmp, "%d", cnt);
		if (enter(tmp, 2) < 0) {
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

int warn(char *s) {
	fprintf(stderr, "warning: %s\n", s);
}

int init_squeue(void) {
	int queue_id;

	if ((queue_id = msgget(SQKEY, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}
	
	return (queue_id);
}