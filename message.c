#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "q.h"

#define SIGINIT 0
#define USERENTERED 1
#define MEMBERCNT 2

#define SNDPID 11
#define GAMESTART 12

#define USERFULL 21

void *run_game(void *arg);
void *rcv_thread(void *arg);
void *snd_thread(void *arg);
int warn(char *s);
int proc_obj(struct q_entry *msg);

pthread_mutex_t mutex;
char *msg;
long rcv_signal = 0; // 1 ~ 10: from server, 11 ~ 20: to server, 21 ~ 30: UI

int main() {
	int signal, select;
	key_t skey, ckey;
	pthread_t ct_id, st_id, gm_id;

	msg = (char*)malloc(sizeof(char)*MAXOBN);
	pthread_mutex_init(&mutex, NULL);

	for (;;) {
		printf("==========[DavinciCode]==========\n");
		printf("1. 멤버 1\n");
		printf("2. 멤버 2\n");
		printf("=================================\n");
		printf("멤버를 선택하세요(1~2): ");
		scanf("%d", &select);

		if (select == 1 || select == 2) {
			break;
		}
		else {
			printf("잘못된 숫자 입니다.\n");
			continue;
		}
	}
	skey = SQKEY + select;
	ckey = CQKEY + select;

	sprintf(msg, "%d", getpid());
	rcv_signal = SNDPID;

	printf("게임 입장 중...\n");
	sleep(3);

	// rcv thread create
	if (pthread_create(&ct_id, NULL, rcv_thread, (void*)&skey) != 0) {
		perror("thread create error");
		return -1;
	}
	// snd thread create
	if (pthread_create(&st_id, NULL, snd_thread, (void*)&ckey) != 0) {
		perror("thread create error");
		return -1;
	}
	// game thread create
	if (pthread_create(&gm_id, NULL, run_game, NULL) != 0) {
		perror("thread create error");
		return -1;
	}

	if (pthread_join(ct_id, NULL) != 0) {
		perror("thread join error");
		return -1;
	}
	if (pthread_join(st_id, NULL) != 0) {
		perror("thread join error");
		return -1;
	}
	if (pthread_join(gm_id, NULL) != 0) {
		perror("thread join error");
		return -1;
	}

	pthread_mutex_destroy(&mutex);
	free(msg);
	exit(0);
}

void *run_game(void *arg) {
	int start;
	
	printf("Davinci Code에 오신 것을 환영합니다.\n");

	printf("다른 멤버를 기다리는 중입니다...\n");
	sleep(1);
	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal > 20) {
			switch (rcv_signal)
			{
			case USERFULL:
				printf("게임을 시작하려면 1을 입력해주세요: ");
				scanf("%d", &start);
				if (start == 1) {
					sprintf(msg, "start");
					rcv_signal = GAMESTART;
				}
				else {
					printf("잘못 입력하셨습니다.\n");
					continue;
				}
				break;
			default:
				break;
			}
		}
		pthread_mutex_unlock(&mutex);
	}
}

void *rcv_thread(void *arg) {
	int mlen, r_qid;
	struct q_entry r_entry;
	key_t skey;
	skey = *((key_t*)arg);

	if ((r_qid = msgget(skey, IPC_CREAT|QPERM)) == -1) {
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
	key_t ckey = *((key_t*)arg);
	int s_qid;

	if ((s_qid = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		return (-1);
	}

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal > 10 && rcv_signal <= 20) {
			if ((len = strlen(msg)) > MAXOBN) {
				warn("string too long");
				return (-1);
			}

			s_entry.mtype = (long)rcv_signal;
			strncpy(s_entry.mtext, msg, len);

			if (msgsnd(s_qid, &s_entry, len, 0) == -1) {
				perror("msgsnd failed");
				return (-1);
			}
			else {
				printf("complete send: %s\n", s_entry.mtext);
			}
			rcv_signal = SIGINIT;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int proc_obj(struct q_entry *rcv) {
	/* receive signal
	 * 1: 유저 입장 알림
	 * 2: 인원 현황 알림
	 * 3:  */
	if (rcv->mtype == USERENTERED) {
		printf("%s 님이 입장했습니다.\n", rcv->mtext);
	}
	else if (rcv->mtype == MEMBERCNT) {
		printf("현재 멤버: %s\n", rcv->mtext);
		if (atoi(rcv->mtext) > 1) {
			sprintf(msg, "client ready...\n");
			rcv_signal = USERFULL;
		}
	}
}

int warn(char *s) {
	fprintf(stderr, "warning: %s\n", s);
}