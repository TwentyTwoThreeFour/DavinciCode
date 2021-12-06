#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define S_SHMKEY (key_t) 60071   /* 서버 공유 메모리 키 */
#define C_SHMKEY (key_t) 60074   /* 클라 공유 메모리 키 */
#define IFLAGS (IPC_CREAT | IPC_EXCL)
#define ERR ((struct databuf *)-1)
#define SIZ 1000 /* 읽기 쓰기를 위한 버퍼의 크기 */

struct databuf {
    int d_nread; /* 데이터 사이즈 */
    char d_buf[SIZ]; /* 데이터 내용 */
};