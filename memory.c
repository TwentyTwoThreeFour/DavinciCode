#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHMKEY1 (key_t) 60071 /* 공유 메모리 키 */
#define SHMKEY2 (key_t) 60072
#define SEMKEY (key_t) 0x7020 /* 세마포어 키 */
#define IFLAGS (IPC_CREAT | IPC_EXCL)
#define ERR ((struct databuf *)-1)
#define SIZ 100 /* 읽기 쓰기를 위한 버퍼의 크기 */

struct databuf {
    int d_nread; /* 데이터 사이즈 */
    char d_buf[SIZ]; /* 데이터 내용 */
};

void writer (int semid, struct databuf *buf1, struct databuf *buf2);
void reader (int semid, struct databuf *buf1, struct databuf *buf2);
void sig_handler (int signo);

static int shmid1, shmid2, semid;
struct sembuf p1={0,-1,0}, p2={1,-1,0};
struct sembuf v1={0,1,0}, v2={1,1,0};

typedef union _semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
} semun;

/* 공유메모리 생성 함수 */
void getseg(struct databuf **p1, struct databuf **p2) {
    /* 공유메모리 영역 생성 */
    if((shmid1 = shmget(SHMKEY1, sizeof(struct databuf), 0600 | IFLAGS)) == -1)
        exit(1);
    if((shmid2 = shmget(SHMKEY2, sizeof(struct databuf), 0600 | IFLAGS)) == -1)
        exit(2);
    /* 공유 메모리 영역 부착 */
    if((*p1 = (struct databuf *) shmat(shmid1, 0, 0)) == ERR)
        exit(3);
    if((*p2 = (struct databuf *) shmat(shmid2, 0, 0)) == ERR)
        exit(4);
}

/* 세마포어 집합 얻는 함수 */
int getsem(void) {
    semun x;
    x.val = 0;

    /* 두 개의 세마포어를 갖는 집합 생성 */
    if((semid = semget(SEMKEY, 2, 0600|IFLAGS)) == -1)
        exit(5);
    if(semctl(semid, 0, SETVAL, x) == -1) //초깃값 지정
        exit(6);
    if(semctl(semid, 1, SETVAL, x) == -1) //초깃값 지정
        exit(7);
    return (semid);
}

/* 공유 메모리 식별자 & 세마포어 집합 식별자 제거 */
void remobj(void) {
    if(shmctl(shmid1, IPC_RMID, NULL) == -1)
        exit(9);
    if(shmctl(shmid2, IPC_RMID, NULL) == -1)
        exit(10);
    if(semctl(semid, IPC_RMID, NULL) == -1)
        exit(11);
}

int main(void) {
    int semid;
    pid_t pid;
    struct databuf *buf1, *buf2;
    //signal(SIGINT, sig_handler);
    
    /* 세마포어 집합 초기화 */
    semid = getsem();
    /* 공유 메모리 영역 생성 및 부착 */
    getseg(&buf1, &buf2);

    switch(pid = fork()) {
        case -1:
            exit(12);
        case 0:     /* 자식 */
            writer(semid, buf1, buf2);
            remobj();
            break;
        default:    /* 부모 */
            reader(semid, buf1, buf2);
            break;
    }
    exit(0);
}

/* 공유 메모리 읽기 */
void reader (int semid, struct databuf *buf1, struct databuf *buf2) {
    for(;;) {
        buf1->d_nread = read(0, buf1->d_buf, SIZ);

        semop(semid, &v1, 1);
        semop(semid, &p2, 1);

        if(buf1->d_nread <= 0) return;

        buf2->d_nread = read(0, buf2->d_buf, SIZ);

        semop(semid, &v1, 1);
        semop(semid, &p2, 1);

        if(buf2->d_nread <= 0) return;
    }
}

/* 공유 메모리 쓰기 */
void writer (int semid, struct databuf *buf1, struct databuf *buf2) {
    for(;;) {
        semop(semid, &p1, 1);
        semop(semid, &v2, 1);

        if(buf1->d_nread <= 0) return;

        write(1, buf1->d_buf, buf1->d_nread);

        semop(semid, &p1, 1);
        semop(semid, &v2, 1);

        if(buf2->d_nread <= 0) return;

        write(1, buf2->d_buf, buf2->d_nread);
    }
}