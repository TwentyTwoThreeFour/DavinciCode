#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define ERR ((struct databuf *)-1)

#define SSHMKEY (key_t)60070	// server basic key
#define CSHMKEY (key_t)60074	// client basic key
#define SHMPERM 0666
#define MAXOBN 32
#define MAXINDEX 4
#define MAXSIGN 10
#define BLOCKNUM 24

#define SIGINIT 0			// signal 초기화

// server -> client signal
#define USERENTERED 1		// 유저 입장
#define MEMBERCNT 2
#define STARTED 3
#define TURN 4
#define TERMINATED 5

// client -> server signal
#define SNDPID 11
#define READY 12
#define BLOCKCOLOR 13
#define SELECTBLOCK 14
#define GUESSBLOCK 15
#define TURNCHANGE 16
#define BLOCKCHECK 17
#define GAMEOVER 18

// client internal signal
#define USERFULL 21
#define GAMESTART 22
#define PLAY 23
#define EXIT 24

// block status
#define NONE 0
#define HAVE 1
#define OPEN 2

struct databuf {
    // int d_nread; /* 데이터 사이즈 */
    long mtype;
    int message;
    int pid;
    int blocks[BLOCKNUM];
    int user1[BLOCKNUM];
    int user2[BLOCKNUM];
    int userstatus;
    int gameover;
};