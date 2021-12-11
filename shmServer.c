#include <time.h>
#include "shm.h"

#define CTHREAD_NUM 2
#define QUEUE_NUM 2

void init_blocks(void);

void *rcv_thread(void *arg);
void *snd_thread(void *arg);

int getssem(key_t semkey);
int getcsem(key_t semkey);

void proc_clientmsg(struct databuf *rcv);
int setting_block(int pid, int select);
int select_block(int pid, int select);
int open_block(int pid, int select);
int check_block(int pid);

pthread_mutex_t mutex;

//semaphore
struct sembuf p={0,-1,0}, v={0, 1, 0}; //p: lock걸기 v:lock풀기

// user manage
int memcnt = 0;			// member count
int member[3];			// member pids
int readycnt = 0;		// ready count

// message
int msg;
long rcv_signal = 0;	// 1 ~ 10: to client, 11 ~ 20: from client

// blocks
int blocks[BLOCKNUM];	// 0 ~ 11: BlackBlock, 12 ~ 23: WhiteBlock
int user1[BLOCKNUM];	// user1 hands
int user2[BLOCKNUM];	// user2 hands

// game manage
// <userstatus>
// user1:[1: 해당 턴, 2: 블록 선택, 3: 상대 블록 선택]
// user2:[4: 해당 턴, 5: 블록 선택, 6: 상대 블록 선택]
int userstatus = 1;
int gameover = 0;	// 1: 게임 종료, 0: 게임 재개

struct keys {
	key_t ckey;
	key_t skey;
	int s_semid;
	int c_semid;
};

int main(void) {
	int pid;
	pthread_t ct_id[CTHREAD_NUM], st_id;			// thread identifier
	struct keys key[2];
	int s_semid;

	key[0].s_semid = getssem(SSEMKEY1);
	key[1].s_semid = getssem(SSEMKEY2);
	key[0].c_semid = getcsem(CSEMKEY1);
	key[1].c_semid = getcsem(CSEMKEY2);

	for (int i = 0; i < 2; i++) {
		key[i].ckey = CSHMKEY + i + 1;
		key[i].skey = SSHMKEY + i + 1;
	}

	// key_t ckey[] = {CSHMKEY + 1, CSHMKEY + 2};			// rcv memory key
	// key_t skey[] = {SSHMKEY + 1, SSHMKEY + 2};			// snd memory key
	
	printf("Davinci Code Server Launch...\n");
	init_blocks();
	
	// s_semid = getssem(); //서버 세마포어 초기화
	// memcpy(c_semid, getcsem(), sizeof(c_semid)); //클라이언트 세마포어 초기화

	// thread setting
	pthread_mutex_init(&mutex, NULL);

	for (int i = 0; i < CTHREAD_NUM; i++) {
		if (pthread_create(&(ct_id[i]), NULL, rcv_thread, (void*)(&key[i])) != 0) {
			perror("thread create error");
			return -1;
		}
	}
	if (pthread_create(&st_id, NULL, snd_thread, (void*)key) != 0) {
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
	exit(0);
}

void init_blocks(void) {
	for (int i = 0; i < BLOCKNUM; i++) {
		blocks[i] = 1;
	} 
	memset(user1, 0, sizeof(user1));
	memset(user2, 0, sizeof(user2));
}

//서버 바이너리 세마포어
key_t getssem(key_t semkey) {
	semun x;
	x.val = 0;
	key_t s_semid;

	if((s_semid = semget(semkey, 1, IPC_CREAT|0666)) == -1) {
		perror("server semget failed");
	}

	if(semctl(s_semid, 0, SETVAL, x) == -1) //초깃값 지정
        perror("server semctl failed");

    return (s_semid);
}

//클라이언트 바이너리 세마포어
int getcsem(key_t semkey) {
	semun x;
	x.val = 0;
	key_t c_semid;

	if((c_semid = semget(semkey, 1, IPC_CREAT|0666)) == -1) {
		perror("server semget failed");
	}
	for (int i = 0; i < 2; i++) {
		if(semctl(c_semid, 0, SETVAL, x) == -1) //초깃값 지정
    		perror("server semctl failed");
	}
	
    return (c_semid);
}


void *rcv_thread(void *arg) {
	int r_mid;
	struct databuf *r_buf;
	struct keys key;
	key = *((struct keys*)arg);
	struct databuf *memory_seg = NULL;

	if ((r_mid = shmget(key.ckey, sizeof(struct databuf), IPC_CREAT|SHMPERM)) == -1) {
		perror("shmget failed");
	}

	if((memory_seg = (struct databuf *)shmat(r_mid, 0, 0)) == ERR) {
		perror("shmat failed");
	}

	for (;;) {
		semop(key.c_semid, &p, 1);
		r_buf = memory_seg;
		proc_clientmsg(r_buf);
	}
}

void *snd_thread(void *arg) {
	int len;
	struct databuf *s_buf;
	struct keys key[QUEUE_NUM];
	int s_mid[QUEUE_NUM];
	struct databuf *memory_seg[QUEUE_NUM] = {NULL, NULL};
	
	// key setting
	for (int i = 0; i < QUEUE_NUM; i++) {
		key[i] = ((struct keys*)arg)[i];
	}

	// init queue
	for (int i = 0; i < QUEUE_NUM; i++) {
		if ((s_mid[i] = shmget(key[i].skey, sizeof(struct databuf), IPC_CREAT|SHMPERM)) == -1) {
			exit(1);
		}

		if((memory_seg[i] = (struct databuf *)shmat(s_mid[i], 0, 0)) == ERR) {
			perror("shmat failed");
		}
	}

	printf("%d %d\n", s_mid[0], s_mid[1]);

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal != 0) {
			for (int i = 0; i < QUEUE_NUM; i++) {
				// msg setting
				s_buf = memory_seg[i];
				s_buf->mtype = (long)rcv_signal;
				s_buf->message = msg;
				s_buf->gameover = gameover;

				// signal이 TURN or TERMINATED일 경우에만 해당 msg setting
				if (rcv_signal == TURN || rcv_signal == TERMINATED) {
					if (userstatus < 4) {
						s_buf->pid = member[0];
					}
					else if (userstatus >= 4) {
						s_buf->pid = member[1];
					}
					s_buf->userstatus = userstatus;
					memcpy(s_buf->blocks, blocks, sizeof(blocks));
					memcpy(s_buf->user1, user1, sizeof(user1));
					memcpy(s_buf->user2, user2, sizeof(user2));
				}
				printf("send complete: %ld\n", memory_seg[i]->mtype);
				semop(key[i].s_semid, &v, 1);
			}
			// variable init
			if (rcv_signal == TERMINATED) {
				exit(0);
			}
			rcv_signal = SIGINIT;
			readycnt = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
}

void proc_clientmsg(struct databuf *rcv) {
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
		break;
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
		break;
	case BLOCKCHECK:
		pthread_mutex_lock(&mutex);
		if ((gameover = check_block(rcv->pid)) == 1) {
			userstatus = rcv->userstatus;
		}
		else {
			if (rcv->userstatus == 3) {
				userstatus = 2;
			}
			else if (rcv->userstatus == 6) {
				userstatus = 5;
			}
		}
		rcv_signal = TURN;
		pthread_mutex_unlock(&mutex);
		break;
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
		break;
	case GAMEOVER:
		pthread_mutex_lock(&mutex);
		userstatus = rcv->userstatus;
		rcv_signal = TERMINATED;
		pthread_mutex_unlock(&mutex);
		break;
	default:
		break;
	}

	printf("signal: %ld msg: %d pid: %d\n", rcv->mtype, rcv->message, rcv->pid);
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
	// select가 1이면 0~11, 2이면 12~23 사이의 숫자 생성
	if (pid == member[0]) {
		for (;;) {
			tmp = rand() % 12 + (12 * (select - 1));
			if (blocks[tmp] == HAVE) {
				blocks[tmp] = NONE;
				user1[tmp] = HAVE;
				return tmp;
			}
		}
	}
	else if (pid == member[1]) {
		for (;;) {
			tmp = rand() % 12 + (12 * (select - 1));
			if (blocks[tmp] == HAVE) {
				blocks[tmp] = NONE;
				user2[tmp] = HAVE;
				return tmp;
			}
		}
	}
	return -1;
}

// 1: 맞춤, 2: 틀림
int open_block(int pid, int select) {
	if (pid == member[0]) {
		if (user2[select] == HAVE) {
			user2[select] = OPEN;
			return 1;
		}
		else if (user2[select] == 0){
			return 2;
		}
	}
	else if (pid == member[1]) {
		if (user1[select] == HAVE) {
			user1[select] = OPEN;
			return 1;
		}
		else if (user1[select] == 0) {
			return 2;
		}
	}
	return -1;
}

// 0: 남은 블록이 존재함, 1: 다 맞춤
int check_block(int pid) {
	if (pid == member[0]) {
		for (int i = 0; i < BLOCKNUM; i++) {
			if (user2[i] == HAVE) {
				return 0;
			}
		}
	}
	else if (pid == member[1]) {
		for (int i = 0; i < BLOCKNUM; i++) {
			if (user1[i] == HAVE) {
				return 0;
			}
		}
	}
	return 1;
}