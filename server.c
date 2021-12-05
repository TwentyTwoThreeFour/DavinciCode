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
		case 0:
			serve();
			break;
		case -1:
			warn("fork to start server failed");
			break;
		default:
			printf("server process pid is %d\n", pid);
	}
	exit(pid != -1 ? 0 : 1);
}

int serve(void) {
	int mlen, r_qid;
	struct q_entry r_entry;

	if ((r_qid = init_queue()) == -1) {
		return (-1);
	}

	for (;;) {

