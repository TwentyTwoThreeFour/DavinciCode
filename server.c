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
int proc_obj(struct q_entry *rcv);
char *substr(int s, int e, char *str);
int enter(char *msg, int signal);

pthread_mutex_t mutex;
int memcnt = 0;
int member[2];
char *msg;
int rcv_signal = 0;

int main(void) {
	int pid;
	pthread_t ct_id[THREAD_NUM], st_id;				// thread identifier
	int ckey[THREAD_NUM] = {CQKEY + 1, CQKEY + 2};	// rcv queue key
	int skey = SQKEY + 1;							// snd queue key

	printf("Davinci Code Server Launched...\n");

	pthread_mutex_init(&mutex, NULL);
	
	// rcv thread create
	for (int i = 0; i < THREAD_NUM; i++) {
		if (pthread_create(&ct_id[i], NULL, rcv_thread, (void*)ckey[i]) != 0) {
			perror("thread create error");
			return -1;
		}
	}

	// snd thread create
	if (pthread_create(&st_id, NULL, serve, (void*)skey) != 0) {
		perror("thread create error");
		return -1;
	}

	for (int i = 0; i < THREAD_NUM; i++) {
		if (pthread_join(ct_id[i], NULL) != 0) {
			perror("thread join error");
			return -1;
		}
	}

	if (pthread_join(st_id, NULL) != 0) {
		perror("thread join error");
		return -1;
	}

	pthread_mutex_destroy(&mutex);
	exit(pid != -1 ? 0 : 1);
}

int *rcv_thread(void *arg) {
	int mlen, r_qid;
	struct q_entry r_entry;
	key_t ckey = *((key_t*)arg);

	if ((r_qid = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}

	for (;;) {
		if (mlen = msgrcv(r_qid, &r_entry, MAXOBN, (-1 * MAXSIGN), MSG_NOERROR) == -1) {
			perror("msgrcv failed");
		}
		else {
			r_entry.mtext[mlen] = '\0';
			
			pthread_mutex_lock(&mutex);
			proc_obj(&r_entry);
			pthread_mutex_unlock(&mutex);
		}
	}
}

int *snd_thread(void *arg) {
	int len, s_qid;
	struct q_entry s_entry;
	key_t skey = *((key_t*)arg);

	if ((len = strlen(msg)) > MAXOBN) {
		warn("string too long");
		return (-1);
	}

	if ((s_qid = init_squeue()) == -1) {
		return (-1);
	}

	if (rcv_signal != 0) {
		s_entry.mtype = (long)rcv_signal;
		strncpy(s_entry.mtext, msg, MAXOBN);

		rcv_signal = 0;
		if (msgsnd(s_qid, &s_entry, strlen(msg), 0) == -1) {
			perror("msgsnd failed");
			return (-1);
		}
		else {
			return (0);
		}
	}
}

int enter(char *msg, int signal) {
	int len, s_qid;
	struct q_entry s_entry;

	if ((len = strlen(msg)) > MAXOBN) {
		warn("string too long");
		return (-1);
	}

	if (signal > MAXSIGN || signal < 0) {
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
		sprintf(msg, "%d", member[memcnt]);
		rcv_signal = 1;

		memcnt++;

		printf("system: %d members\n", memcnt);
		sprintf(msg, "%d", memcnt);
		rcv_signal = 2;
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