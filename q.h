#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#define SQKEY (key_t)60070	// server basic key
#define CQKEY (key_t)60074	// client basic key
#define QPERM 0660
#define MAXOBN 100
#define MAXINDEX 4
#define MAXSIGN 10

struct q_entry {
	long mtype;
	char mtext[MAXOBN+1];
};
