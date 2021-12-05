#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#define QKEY (key_t)0105
#define QPERM 0660
#define MAXOBN 100
#define MAXPRIOR 10

struct q_entry {
	long mtype;
	char mtext[MAXOBN+1];
};
