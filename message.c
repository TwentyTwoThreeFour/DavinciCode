#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "q.h"

void run_game(void);
int enter(char *objname, int priority);
int warn(char *s);
int init_queue(void);
int proc_obj(struct q_entry *msg);

int main() {
	int signal;
	
	
	exit(0);
}

void run_game(void) {
	int mlen, qid;
	struct q_entry s_entry, r_entry; 

	if ((qid = init_queue()) == -1) {
		return (-1);
	}

	enter(getpid(), 1);
	printf("Davinci Code에 오신 것을 환영합니다.\n");

	pritnf("다른 멤버를 기다리는 중입니다...\n");
	for (;;) {
		if ((mlen = msgrcv(qid, &r_entry, MAXOBN, 1, MSG_NOERROR)) == -1) {
			perror("msgrcv failed");
		}
		else {
			r_entry.mtext[mlen] = '\0';

			proc_obj(&r_entry);
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

		}
	}

	printf("priority: %ld name: %s\n", msg->mtype, msg->mtext);
}

int enter(char *objname, int priority) {
	int len, s_qid;
	struct q_entry s_entry;

	if ((len = strlen(objname)) > MAXOBN) {
		warn("name too long");
		return (-1);
	}

	if (priority > MAXPRIOR || priority < 0) {
		warn("invalid priority level");
		return (-1);
	}

	if ((s_qid = init_queue()) == -1) {
		return (-1);
	}

	s_entry.mtype = (long)priority;
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

int init_queue(void) {
	int queue_id;

	if ((queue_id = msgget(QKEY, IPC_CREAT|QPERM)) == -1) {
		perror("msgget failed");
	}
	
	return (queue_id);
}