#include <string.h>
#include <time.h>
#include "q.h"
#include <fcntl.h>
#include <sys/stat.h>

#define CTHREAD_NUM 2
#define QUEUE_NUM 2

void init_blocks(void);

void *rcv_thread(void *arg);
void *snd_thread(void **arg);

void proc_clientmsg(struct q_entry *rcv);

int setting_block(int pid, int select);
int select_block(int pid, int select);
int open_block(int pid, int select);
int check_block(int pid);

pthread_mutex_t mutex;

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

int main(void) {
	int pid;
	pthread_t ct_id[CTHREAD_NUM], st_id;			// thread identifier
	char *rpath[] = {"./FIFO1", "./FIFO2"};			// rcv pipe path
	char *spath[] = {"./FIFO3", "./FIFO4"};			// snd pipe path


	if(mkfifo("./FIFO1", 0666)==-1){
		printf("fail to call fifo()\n");
	}	//rcv pipe client1

	if(mkfifo("./FIFO2", 0666)==-1){
		printf("fail to call fifo()\n");
	}	//rcv pipe client2
	
	if(mkfifo("./FIFO3", 0666)==-1){
		printf("fail to call fifo()\n");
	}	//snd pipe client1
	
	if(mkfifo("./FIFO4", 0666)==-1){
		printf("fail to call fifo()\n");
	}	//snd pipe client2
	
	
	printf("Davinci Code Server Launch...\n");
	init_blocks();
	
	// thread setting
	pthread_mutex_init(&mutex, NULL);

	for (int i = 0; i < CTHREAD_NUM; i++) {
		if (pthread_create(&(ct_id[i]), NULL, rcv_thread, (void*)(rpath[i])) != 0) {
			perror("thread create error");
			return -1;
		}
	}
	if (pthread_create(&st_id, NULL, snd_thread, (void**)spath) != 0) {
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

void *rcv_thread(void *arg) {
	int mlen, r_qid;
	struct q_entry r_entry;
	char *path;
	path = (char *)arg;
	int fd;

	printf("%s\n", path);
	if((fd = open(path, O_RDONLY)) <0){
		printf("rcv: open fifo faild\n");
		exit(1);
	}

	for (;;) {
		if(read(fd, &r_entry, sizeof(struct q_entry)) == -1){
			perror("msgrcv failed");
		}
		
		else {
			proc_clientmsg(&r_entry);
		}
	}
	close(fd);
}

void *snd_thread(void **arg) {
	int len;
	struct q_entry s_entry;
	char *spath[QUEUE_NUM];
	int s_qid[QUEUE_NUM];
	int fd[QUEUE_NUM];

	for (int i = 0; i < QUEUE_NUM; i++) {
		spath[i] = (char *)arg[i];
	}
	
	for (int i = 0; i < QUEUE_NUM; i++) {
		if((fd[i] = open(spath[i], O_WRONLY))<0){
			printf("snd: 3open fifo faild\n");
			exit(1);
		}	
	}

	for (;;) {
		pthread_mutex_lock(&mutex);
		if (rcv_signal != 0) {
			// msg setting
			s_entry.mtype = (long)rcv_signal;
			s_entry.message = msg;
			s_entry.gameover = gameover;

			// signal이 TURN or TERMINATED일 경우에만 해당 msg setting
			if (rcv_signal == TURN || rcv_signal == TERMINATED) {
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
			// snd for clients

			for (int i = 0; i < QUEUE_NUM; i++) {
				if (write(fd[i], &s_entry, sizeof(struct q_entry)) == -1) {
					perror("msgsnd failed");
					exit(3);
				}
				else {
					printf("send complete: %ld\n", s_entry.mtype);
				}
			}
			// variable init
			rcv_signal = SIGINIT;
			readycnt = 0;
		}
		pthread_mutex_unlock(&mutex);
	}
	for (int i = 0; i < QUEUE_NUM; i++) {
		close(fd[i]);
	}
}

void proc_clientmsg(struct q_entry *rcv) {
	switch (rcv->mtype)
	{
	case SNDPID:
		pthread_mutex_lock(&mutex);
		if (memcnt < 2) {
			member[memcnt] = rcv->pid;
			printf("system: %d entered\n", member[memcnt]);
			msg = member[memcnt];
			rcv_signal = USERENTERED;
			memcnt++;
		}
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
