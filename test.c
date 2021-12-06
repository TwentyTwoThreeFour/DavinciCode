#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "q.h"

int serve(void);
int warn(char *s);
int init_queue(void);
int proc_obj(struct q_entry *msg);

int main(void) {
    int pid;
    
    switch (pid = fork()) {
    case 0:		/* 자식 */
        serve();
        break;
    case -1:		/* 실제로는, 서버는 결코 퇴장(exit)하지 않음 */
        warn("fork to start server failed");
        break;
    default:
        printf("server process pid is %d\n", pid);
    }
    exit(pid != -1 ? 0 : 1);
}

/* serve -- queue에서 가장 우선 순위가 높은 객체를 처리한다. */
int serve(void) {
    int mlen, r_qid;
    struct q_entry r_entry;
    
    /* 필요에 따라 message queue를 초기화한다. */
    if ((r_qid = init_queue()) == -1) {
        return (-1);
    }
    
    /* 다음 message를 가져와 처리한다. 필요하면 기다린다. */
    for (;;) {
        if ((mlen = msgrcv(r_qid, &r_entry, MAXOBN, 0, MSG_NOERROR)) == -1) {
            perror("msgrcv failed");
            return (-1);
        }
        else {
            /* 우리가 문자열을 가지고 있는지 확인한다. */
            r_entry.mtext[mlen] = '\0';
            
            /* 객체 이름을 처리한다 */
            proc_obj(&r_entry);
        }
    }
}

int warn(char *s) {
    fprintf(stderr, "warning: %s\n", s);
}

/* init_queue -- queue identifier를 획득한다 */
int init_queue(void) {
    int queue_id;
    
    /* message queue를 생성하거나 개방하려고 시도한다. */
    if ((queue_id = msgget(CQKEY+1, IPC_CREAT|QPERM)) == -1) {
        perror("msgget failed");
    }
    
	printf("%d", queue_id);
    return (queue_id);
}

int proc_obj(struct q_entry *msg) {
    printf("\npriority: %ld name: %s\n", msg->mtype, msg->mtext);
}