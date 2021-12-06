#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "q.h"

#define CTHREAD_NUM 2
#define QUEUE_NUM 2

// struct user {
// 	int hands[13];
// };

void *rcv_thread(void *arg);
void *snd_thread(void *arg);
int warn(char *s);
int init_squeue(key_t skey);
int proc_obj(struct q_entry *rcv);
char *substr(int s, int e, char *str);

pthread_mutex_t mutex;
int memcnt = 0;
int member[3];
char *msg;
long rcv_signal = 0;

int main(void) {
	int pid;
	pthread_t ct_id[CTHREAD_NUM], st_id;				// thread identifier
	key_t ckey[] = {CQKEY + 1, CQKEY + 2};			// rcv queue key
	key_t skey[] = {SQKEY + 1, SQKEY + 2};			// snd queue key

	printf("Davinci Code Server Launch...\n");
	msg = (char*)malloc(sizeof(char)*MAXOBN);
	pthread_mutex_init(&mutex, NULL);
	
	// rcv thread create
	for (int i = 0; i < CTHREAD_NUM; i++) {
		if (pthread_create(&(ct_id[i]), NULL, rcv_thread, (void*)(&ckey[i])) != 0) {
			perror("thread create error");
			return -1;
		}
	}

	// snd thread create
	if (pthread_create(&st_id, NULL, snd_thread, (void*)skey) != 0) {
		perror("thread create error");
		return -1;
	}
	printf("complete create thread\n");

	for (int i = 0; i < CTHREAD_NUM; i++) {
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
	free(msg);
	exit(pid != -1 ? 0 : 1);
}

void *rcv_thread(void *arg) {
	int mlen, r_qid;
	struct q_entry r_entry;
	key_t ckey;
	ckey = *((key_t*)arg);
	
	if ((r_qid = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}

	for (;;) {
		if ((mlen = msgrcv(r_qid, &r_entry, MAXOBN, 0, MSG_NOERROR)) == -1) {
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

void *snd_thread(void *arg) {
	int len;
	struct q_entry s_entry;
	key_t skey[QUEUE_NUM];
	int s_qid[QUEUE_NUM];
	
	for (int i = 0; i < QUEUE_NUM; i++) {
		skey[i] = ((key_t*)arg)[i];
	}

	for (int i = 0; i < QUEUE_NUM; i++) {
		if ((s_qid[i] = init_squeue(skey[i])) == -1) {
			exit(1);
		}
	}

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal != 0) {
			if ((len = strlen(msg)) > MAXOBN) {
				warn("string too long");
				exit(2);
			}

			s_entry.mtype = (long)rcv_signal;
			strncpy(s_entry.mtext, msg, MAXOBN);

			for (int i = 0; i < QUEUE_NUM; i++) {
				if (msgsnd(s_qid[i], &s_entry, len, 0) == -1) {
					perror("msgsnd failed");
					exit(3);
				}
			}
			rcv_signal = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int proc_obj(struct q_entry *rcv) {
	/* receive signal
	 * 11: 클라이언트 진입 
	 * 12: 게임 시작 여부
	 * 3: 블록 색 선정
	 * 4: 조커 위치 선정(필요 시) */
	if (rcv->mtype == 11) {
		member[memcnt] = atoi(rcv->mtext);

		printf("system: %d entered\n", member[memcnt]);
		sprintf(msg, "%d", member[memcnt]);
		rcv_signal = 1;

		memcnt++;

		printf("system: %d members\n", memcnt);
		sprintf(msg, "%d", memcnt);
		rcv_signal = 2;
	}
	else if (rcv->mtype == 12) {
		printf("%s", rcv->mtext);
	}

	printf("signal: %ld msg: %s\n", rcv->mtype, rcv->mtext);
}

char *substr(int s, int e, char *str) {
	char *new = (char *)malloc(sizeof(char)*(e-s+2));
	strncpy(new, str+s, e-s+1);
}

int warn(char *s) {
	fprintf(stderr, "warning: %s\n", s);
}

int init_squeue(key_t skey) {
	int queue_id;

	if ((queue_id = msgget(skey, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}
	
	return (queue_id);
}