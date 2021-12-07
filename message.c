#include "q.h"


#define SIGINIT 0			// signal 초기화

// from server signal
#define USERENTERED 1		// 유저 입장
#define MEMBERCNT 2
#define STARTED 3
#define TURN 4

// to server signal
#define SNDPID 11
#define READY 12
#define BLOCKCOLOR 13
#define SELECTBLOCK 14
#define GUESSBLOCK 15
#define TURNCHANGE 16

// internal signal
#define USERFULL 21
#define GAMESTART 22
#define PLAY 23

void *run_game(void *arg);
void *rcv_thread(void *arg);
void *snd_thread(void *arg);
int warn(char *s);
int proc_rcv(struct q_entry *msg);

pthread_mutex_t mutex;
int msg;
 // 1 ~ 10: from server, 11 ~ 20: to server, 21 ~ 30: interface
long rcv_signal = 0;

int turn = 0;
int pid;
int blocks[BLOCKNUM]; // 0 ~ 11: black, 12 ~ 23: white, 24: blackjoker, 25: whitejoker
int user1[BLOCKNUM];
int user2[BLOCKNUM];
int userstatus;

int main() {
	int signal, select;
	key_t skey, ckey;
	pthread_t ct_id, st_id, gm_id;

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

	rcv_signal = SNDPID;

	printf("게임 입장 중...\n");
	sleep(1);

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
	exit(0);
}

void *run_game(void *arg) {
	int start;
	int blockcolor, oppoblock;
	
	printf("Davinci Code에 오신 것을 환영합니다.\n");

	printf("다른 멤버를 기다리는 중입니다...\n");
	sleep(1);
	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal > 20) {		// internal signal 일 경우
			switch (rcv_signal)
			{
			case USERFULL:
				printf("게임을 준비하려면 1을 입력해주세요: ");
				scanf("%d", &start);
				if (start == 1) {
					rcv_signal = READY;
				}
				else {
					printf("잘못 입력하셨습니다.\n");
					continue;
				}
				break;
			case GAMESTART:
				printf("=====[블록 색 조합을 선택해주세요]=====\n");
				printf("1. 흰색 4개\n");
				printf("2. 흰색 3개, 검은색 1개\n");
				printf("3. 흰색 2개, 검은색 2개\n");
				printf("4. 흰색 1개, 검은색 3개\n");
				printf("5. 검은색 4개\n");
				printf("================================\n");
				printf("선택: ");
				scanf("%d", &blockcolor);
				if (blockcolor >=1 && blockcolor <=5) {
					msg = blockcolor;
					rcv_signal = BLOCKCOLOR;
				}
				else {
					printf("잘못 입력하셨습니다.\n");
					continue;
				}
				break;
			case PLAY:
				if (pid == getpid()) {
					if (userstatus < 4) {
						printf("yours: ");
						for (int i = 0; i < BLOCKNUM / 2; i++) {
							if (user1[i] == 1 || user1[i] == 2) {
								printf("B%d ", i);
							}
							if (user1[i + 12] == 1 || user1[i + 12] == 2) {
								printf("W%d ", i);
							}
						}
						printf("\noppo's: ");
						for (int i = 0; i < BLOCKNUM / 2; i++) {
							if (user2[i] == 1) {
								printf("(B) ");
							}
							else if (user2[i] == 2) {
								printf("B%d ", i);
							}
							if (user2[i + 12] == 1) {
								printf("(W) ", i);
							}
							else if (user2[i + 12] == 1) {
								printf("W%d ", i);
							}
						}
						printf("\n");
						if (userstatus == 1) {
							for (;;) {
								printf("--------------------------\n");
								printf("어떤 색의 블록을 가져오시겠습니까?\n");
								printf("1. black\n");
								printf("2. white\n");
								printf("--------------------------\n");
								printf("입력: ");
								scanf("%d", &blockcolor);
								if (blockcolor == 1 || blockcolor == 2) {
									msg = blockcolor;
									rcv_signal = SELECTBLOCK;
									break;
								}
							}
						}
						else if (userstatus == 2) {
							for(;;) {
								printf("--------------------------\n");
								printf("상대 블록을 선택해주세요\n");
								printf("1. black(0 ~ 11)\n");
								printf("2. white(12 ~ 23)\n");
								printf("--------------------------\n");
								printf("입력: ");
								scanf("%d", &oppoblock);
								if (oppoblock >= 0 && oppoblock <= 23) {
									msg = oppoblock;
									rcv_signal = GUESSBLOCK;
									break;
								}
							}
						}
						else if (userstatus == 3) {
							if (msg == 1) {
								printf("축하합니다! 정답입니다!\n");
							}
							else if (msg == 2) {
								printf("아쉽네요.. 틀렸습니다.\n");
							}
							printf("턴이 상대에게로 넘어갑니다.\n");
							rcv_signal = TURNCHANGE;
						}
					}
					else if (userstatus >= 4) {
						printf("yours: ");
						for (int i = 0; i < BLOCKNUM / 2; i++) {
							if (user2[i] == 1 || user2[i] == 2) {
								printf("B%d ", i);
							}
							if (user2[i + 12] == 1 || user2[i + 12] == 2) {
								printf("W%d ", i);
							}
						}
						printf("\noppo's: ");
						for (int i = 0; i < BLOCKNUM / 2; i++) {
							if (user1[i] == 1) {
								printf("(B) ");
							}
							else if (user1[i] == 2) {
								printf("B%d ", i);
							}
							if (user1[i + 12] == 1) {
								printf("(W) ");
							}
							else if (user1[i + 12] == 2) {
								printf("W%d ", i);
							}
						}
						printf("\n");
						if (userstatus == 4) {
							for (;;) {
								printf("--------------------------\n");
								printf("어떤 색의 블록을 가져오시겠습니까?\n");
								printf("1. black\n");
								printf("2. white\n");
								printf("--------------------------\n");
								printf("입력: ");
								scanf("%d", &blockcolor);
								if (blockcolor == 1 || blockcolor == 2) {
									msg = blockcolor;
									rcv_signal = SELECTBLOCK;
									break;
								}
							}
						}
						else if (userstatus == 5) {
							for(;;) {
								printf("--------------------------\n");
								printf("상대 블록을 선택해주세요\n");
								printf("1. black(0 ~ 11)\n");
								printf("2. white(12 ~ 23)\n");
								printf("--------------------------\n");
								printf("입력: ");
								scanf("%d", &oppoblock);
								if (oppoblock >= 0 && oppoblock <= 23) {
									msg = oppoblock;
									rcv_signal = GUESSBLOCK;
									break;
								}
							}
						}
						else if (userstatus == 6) {
							if (msg == 1) {
								printf("축하합니다! 정답입니다!\n");
							}
							else if (msg == 2) {
								printf("아쉽네요.. 틀렸습니다.\n");
							}
							printf("턴이 상대에게로 넘어갑니다.\n");
							rcv_signal = TURNCHANGE;
						}
					}
				}
				else {
				}
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
	key_t ckey = *((key_t*)arg);
	int s_qid;

	if ((s_qid = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		return (-1);
	}

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal > 10 && rcv_signal <= 20) {		// to server signal일 경우

			s_entry.mtype = (long)rcv_signal;
			s_entry.pid = getpid();
			s_entry.message = msg;
			s_entry.userstatus = userstatus;

			if (msgsnd(s_qid, &s_entry, sizeof(struct q_entry), 0) == -1) {
				perror("msgsnd failed");
				return (-1);
			}
			else {
				printf("send complete sig: %d\n", rcv_signal);
			}
			rcv_signal = SIGINIT;
		}
		pthread_mutex_unlock(&mutex);
	}
}

int proc_rcv(struct q_entry *rcv) {
	/* receive signal
	 * USERENTERED: 유저 입장 알림
	 * MEMBERCNT: 인원 현황 알림
	 * STARTED: 게임 시작 알림 
	 * TRUN: 턴 시작 알림 */
	switch (rcv->mtype)
	{
	case USERENTERED:
		pthread_mutex_lock(&mutex);
		sleep(2);
		printf("%d 님이 입장했습니다.\n", rcv->message);
		pthread_mutex_unlock(&mutex);
		break;
	case MEMBERCNT:
		pthread_mutex_lock(&mutex);
		printf("현재 멤버: %d\n", rcv->message);
		if (rcv->message > 1) {
			rcv_signal = USERFULL;
		}
		pthread_mutex_unlock(&mutex);
		break;
	case STARTED:
		pthread_mutex_lock(&mutex);
		rcv_signal = GAMESTART;
		pthread_mutex_unlock(&mutex);
		break;
	case TURN:
		pthread_mutex_lock(&mutex);
		userstatus = rcv->userstatus;
		pid = rcv->pid;
		msg = rcv->message;
		memcpy(blocks, rcv->blocks, sizeof(rcv->blocks));
		memcpy(user1, rcv->user1, sizeof(rcv->user1));
		memcpy(user2, rcv->user2, sizeof(rcv->user2));
		rcv_signal = PLAY;
		pthread_mutex_unlock(&mutex);
		break;
	default:
		break;
	}
}

int warn(char *s) {
	fprintf(stderr, "warning: %s\n", s);
}