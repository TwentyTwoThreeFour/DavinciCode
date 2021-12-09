#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define SQKEY (key_t)60070	// server basic key
#define CQKEY (key_t)60074	// client basic key
#define QPERM 0666
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

/* <server -> client signal>
* USERENTERED: 입장한 클라이언트
* MEMBERCNT: 현재 인원
* STARTED: 게임 시작
* TURN: 턴 시작
* TERMINATED: 클라이언트 종료
* 
* <client -> server signal>
* SNDPID: 클라이언트 진입 
* READY: 준비 완료 여부
* BLOCKCOLOR: 초기 블록 색 선정
* SELECTBLOCK: 게임 중 블록 선택
* GUESSBLOCK: 상대 블록 추측
* BLOCKCHECK: 못 맞춘 블록이 있는지 확인
* TURNCHANGE: 턴 변경
* GAMEOVER: 게임 종료
* 
* <client internal signal>
* USERFULL: 인원 꽉참
* GAMESTART: 게임 초기 인터페이스
* PLAY: 게임 진행 인터페이스
* EXIT: 종료
* 
* */

struct q_entry {
	long mtype;
	int message;
	int pid;
	int blocks[BLOCKNUM];
	int user1[BLOCKNUM];
	int user2[BLOCKNUM];
	int userstatus;
	int gameover;
};
