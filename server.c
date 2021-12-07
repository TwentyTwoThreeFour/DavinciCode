#include <string.h>
#include <time.h>
#include "q.h"

#define CTHREAD_NUM 2
#define QUEUE_NUM 2

#define SIGINIT 0			// signal 초기화

// to client signal
#define USERENTERED 1		// 유저 입장
#define MEMBERCNT 2
#define STARTED 3
#define TURN 4

// from client signal
#define SNDPID 11
#define READY 12
#define BLOCKCOLOR 13
#define SELECTBLOCK 14
#define GUESSBLOCK 15
#define TURNCHANGE 16



void *rcv_thread(void *arg);
void *snd_thread(void *arg);
int warn(char *s);
int init_squeue(key_t skey);
int proc_rcv(struct q_entry *rcv);
char *substr(int s, int e, char *str);
int setting_block(int pid, int select);
int select_block(int pid, int select);
int open_block(int pid, int select);

pthread_mutex_t mutex;
// users
int memcnt = 0;
int member[3];
int readycnt = 0;

// message
int msg;
long rcv_signal = 0;

// blocks
int blocks[BLOCKNUM]; // 0 ~ 11: black, 12 ~ 23: white, 24: blackjoker, 25: whitejoker
int user1[BLOCKNUM];
int user2[BLOCKNUM];
int userstatus = 1; // 0: 시작 전, user1[1:해당 턴, 2: 블록 선택, 3: 상대 블록 선택] user2:[4: 해당 턴, 5: 블록 선택, 6: 상대 블록 선택]

int main(void) {
	int pid;
	pthread_t ct_id[CTHREAD_NUM], st_id;				// thread identifier
	key_t ckey[] = {CQKEY + 1, CQKEY + 2};			// rcv queue key
	key_t skey[] = {SQKEY + 1, SQKEY + 2};			// snd queue key
	

	printf("Davinci Code Server Launch...\n");
	for (int i = 0; i < BLOCKNUM; i++) {
		blocks[i] = 1;
	} 
	memset(user1, 0, sizeof(user1));
	memset(user2, 0, sizeof(user2));
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
		if ((mlen = msgrcv(r_qid, &r_entry, sizeof(struct q_entry), 0, MSG_NOERROR)) == -1) {
			perror("msgrcv failed");
		}
		else {
			proc_rcv(&r_entry);
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
			s_entry.mtype = (long)rcv_signal;
			s_entry.message = msg;

			if (rcv_signal == TURN) {
				if (userstatus < 4) {
					s_entry.pid = member[0];
				}
				else if (userstatus >= 4) {
					s_entry.pid = member[1];
				}
				s_entry.userstatus = userstatus;
				memcpy(s_entry.blocks, blocks, sizeof(blocks));
				memcpy(s_entry.user1, user1, sizeof(user1));
				memcpy(s_entry.user2, user2, sizeof(user2));
			}
			for (int i = 0; i < QUEUE_NUM; i++) {
				if (msgsnd(s_qid[i], &s_entry, sizeof(struct q_entry), 0) == -1) {
					perror("msgsnd failed");
					exit(3);
				}
				else {
					printf("send complete: %d\n", s_entry.mtype);
				}
			}
			rcv_signal = SIGINIT;
			readycnt = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int proc_rcv(struct q_entry *rcv) {
	/* receive signal
	 * SNDPID: 클라이언트 진입 
	 * READY: 준비 완료 여부
	 * BLOCKCOLOR: 초기 블록 색 선정
	 * SELECTBLOCK: 게임 중 블록 색 선정
	 * 
	 * send signal
	 * 1: 입장한 client
	 * 2: 현재 인원
	 * 3: 게임 시작
	 * 4: 턴 시작 */
	switch (rcv->mtype)
	{
	case SNDPID:
		pthread_mutex_lock(&mutex);
		member[memcnt] = rcv->pid;
		printf("system: %d entered\n", member[memcnt]);
		msg = member[memcnt];
		rcv_signal = USERENTERED;
		memcnt++;
		pthread_mutex_unlock(&mutex);

		sleep(3);
		pthread_mutex_lock(&mutex);
		printf("system: %d members\n", memcnt);
		msg = memcnt;
		rcv_signal = MEMBERCNT;
		pthread_mutex_unlock(&mutex);
		break;
	case READY:
		pthread_mutex_lock(&mutex);
		printf("%d ready\n", rcv->pid);
		readycnt++;				// 준비 완료 인원 check

		if (readycnt == 2) {	// ready 완료된 인원이 2명일 경우
			rcv_signal = STARTED;		// 게임 시작 signal
		}
		pthread_mutex_unlock(&mutex);
		break;
	case BLOCKCOLOR:
		pthread_mutex_lock(&mutex);
		setting_block(rcv->pid, rcv->message);
		readycnt++;

		if (readycnt == 2) {
			rcv_signal = TURN;
		}

		pthread_mutex_unlock(&mutex);
		break;
	case SELECTBLOCK:
		pthread_mutex_lock(&mutex);
		msg = select_block(rcv->pid, rcv->message);
		if (rcv->userstatus == 1) {
			userstatus = 2;
		}
		else if (rcv->userstatus == 4) {
			userstatus = 5;
		}

		rcv_signal = TURN;
		pthread_mutex_unlock(&mutex);
	case GUESSBLOCK:
		pthread_mutex_lock(&mutex);
		msg = open_block(rcv->pid, rcv->message);
		if (rcv->userstatus == 2) {
			userstatus = 3;
		}
		else if (rcv->userstatus == 5) {
			userstatus = 6;
		}
		rcv_signal = TURN;
		pthread_mutex_unlock(&mutex);
	case TURNCHANGE:
		pthread_mutex_lock(&mutex);
		if (rcv->userstatus == 3) {
			userstatus = 4;
		}
		else if (rcv->userstatus == 6) {
			userstatus = 1;
		}
		rcv_signal = TURN;
		pthread_mutex_unlock(&mutex);
	default:
		break;
	}

	printf("signal: %ld msg: %d pid: %d\n", rcv->mtype, rcv->message, rcv->pid);
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

int setting_block(int pid, int select) {
	srand(time(NULL));
	int tmp;
	if (pid == member[0]) {
		for(int i = 0; i < 5 - select;) {
			tmp = rand() % 12 + 12;
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user1[tmp] = 1;
				i++;
			}
		}
		for(int j = 0; j < select - 1;) {
			tmp = rand() % 12;
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user1[tmp] = 1;
				j++;
			}
		}
		return 1;
	}
	else if (pid == member[1]) {
		for(int i = 0; i < 5 - select;) {
			tmp = rand() % 12 + 12;
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user2[tmp] = 1;
				i++;
			}
		}
		for(int j = 0; j < select - 1;) {
			tmp = rand() % 12;
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user2[tmp] = 1;
				j++;
			}
		}
		return 2;
	}
	return -1;
}

int select_block(int pid, int select) {
	srand(time(NULL));
	int tmp;
	if (pid == member[0]) {
		for (;;) {
			tmp = rand() % 12 + (12 * (select - 1));
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user1[tmp] = 1;
				return tmp;
			}
		}
	}
	else if (pid == member[1]) {
		for (;;) {
			tmp = rand() % 12 + (12 * (select - 1));
			if (blocks[tmp] == 1) {
				blocks[tmp] = 0;
				user2[tmp] = 1;
				return tmp;
			}
		}
	}
	return -1;
}

int open_block(int pid, int select) {
	if (pid == member[0]) {
		if (user2[select] == 1) {
			user2[select] = 2;
			return 1;
		}
		else if (user2[select] == 0){
			return 2;
		}
	}
	else if (pid == member[1]) {
		if (user1[select] == 1) {
			user1[select] = 2;
			return 1;
		}
		else if (user1[select] == 0) {
			return 2;
		}
	}
}