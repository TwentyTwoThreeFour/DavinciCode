#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "q.h"

int run_game(key_t skey, key_t ckey);
int enter(key_t ckey, char *objname, int priority);
int warn(char *s);
int init_squeue(void);
int init_cqueue(void);
int proc_obj(struct q_entry *msg);

int main() {
	int signal, id;
	key_t skey, ckey;
	int select;

	for (;;) {
		printf("==========[DavinciCode]==========\n");
		printf("1. 멤버 1\n");
		printf("2. 멤버 2\n");
		printf("3. 멤버 3\n");
		printf("=================================\n");
		printf("멤버를 선택하세요(1~3): ");
		scanf("%d", &select);

		if (select > 3 || select < 1) {
			continue;
		}
	}
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s objname priority\n", argv[0]);
		exit(1);
	}
	if ((id = atoi(argv[1])) <= 0 || priority > MAXINDEX) {
		warn("invalid id");
		exit(2);
	}

	skey = SQKEY + atoi(id);
	ckey = CQKEY + atoi(id);

	run_game(skey, ckey);
	exit(0);
}

int run_game(key_t skey, key_t ckey) {
	int mlen, r_qid;
	struct q_entry s_entry, r_entry; 
	char *pid;

	if ((r_qid = init_squeue(skey)) == -1) {
		return (-1);
	}

	sprintf(pid, "%d", getpid());
	if (enter(ckey, pid, 1) < 0) {
		warn("send failure");
	}
	printf("Davinci Code에 오신 것을 환영합니다.\n");

	printf("다른 멤버를 기다리는 중입니다...\n");
	sleep(1);
	for (;;) {
		if ((mlen = msgrcv(r_qid, &r_entry, MAXOBN, (-1 * MAXPRIOR), MSG_NOERROR)) == -1) {
			perror("msgrcv failed");
		}
		else {
			r_entry.mtext[mlen] = '\0';

			if (proc_obj(&r_entry) == -1) {
				break;
			}
		}
	}
}

int proc_obj(struct q_entry *msg) {
	/* receive signal
	 * 1: 유저 입장 알림
	 * 2: 인원 현황 알림
	 * 3: 조커 위치 선정(필요 시) */
	if (msg->mtype == 1) {
		printf("%s 님이 입장했습니다.\n", msg->mtext);
	}
	else if (msg->mtype == 2) {
		printf("현재 멤버: %s\n", msg->mtext);
		if (atoi(msg->mtext) > 1) {
			char *start;
			printf("게임을 시작하려면 start를 입력해주세요: ");
			scanf("%s", start);

			if (strcmp("start", start) == 0) {
				return -1;
			}
		}
	}
}

int enter(key_t ckey, char *objname, int signal) {
	int len, s_qid;
	struct q_entry s_entry;

	if ((len = strlen(objname)) > MAXOBN) {
		warn("name too long");
		return (-1);
	}

	if (signal > MAXSIGN || signal < 0) {
		warn("invalid signal");
		return (-1);
	}

	if ((s_qid = init_cqueue(ckey)) == -1) {
		return (-1);
	}

	s_entry.mtype = (long)signal;
	strncpy(s_entry.mtext, objname, MAXOBN);

	if(msgsnd(s_qid, &s_entry, len, 0) == -1) {
		perror("msgsnd failed");
		return (-1);
	}
	else {
		return (0);
	}
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

int init_cqueue(key_t ckey) {
	int queue_id;

	if ((queue_id = msgget(ckey, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}
	
	return (queue_id);
}