#include <sys/shm.h> 
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>

#define CLK_TCK sysconf(_SC_CLK_TCK)	// 시간 단위 Clock Tick

#define MAX_PROCESS_SIZE 100	// 프로세스 개수
#define MAX_LIST_SIZE 10000000	// 정렬할 배열 크기
#define MAX_LOOP_SIZE 100000000	// 연산 반복수
#define MAX_PRINT 100	// print할 배열 요소 개수

typedef struct WorkType{
    int works;	// 연산되는 배열 수
    int loops;	// 연산 반복수
} WorkType;

typedef struct ListHelper{
    int size;	// 리스트 크기
    int p;		// 리스트의 현재 인덱스
} ListHelper;

sem_t* sem;	// semaphore 객체
ListHelper* list_h;	// ListHelper 구조체 객체
int* shm_list;	// 리스트 객체

/* 리스트 초기화 함수 */
void list_init(int list_size) {
    list_h->size = list_size;	// 인자로 넘겨받은 리스트 사이즈로 설정
    list_h->p = 0;		// 초기 인덱스 0으로 설정
}

/* 리스트 출력 함수 */
void print_list()		// MAX_PRINT만큼 리스트 요소 출력
{
    printf("Sorted List: [");
    for(int i=0; i<list_h->p; i++) {
        if (i >= MAX_PRINT) {
            printf("...");
            break;
        }
        printf("%d, ", shm_list[i]);
    }
    printf("]\n");
}

/* 리스트 정렬 함수 */
void sort(int input)	//input: 리스트에 넣어줄 숫자
{
    int i, j, temp;
    
    if (list_h->p >= list_h->size)
        return;
    
    sem_wait(sem);		// semaphore 잠금
    shm_list[list_h->p] = input;
    list_h->p++;
    
    for(i=list_h->p-1; i>0; i--){	// 버블 정렬
        for(j=0; j<i; j++){
            if(shm_list[j]>shm_list[j+1]){
                temp = shm_list[j];
                shm_list[j] = shm_list[j+1];
                shm_list[j+1] = temp;
            }
        }
    }
    sem_post(sem);		// semaphore 잠금 해제
}

/* 계산 함수 */
int calc(int loops)
{
    int res=0, i;
    
    for (i=0; i<loops; i++) {	// 반복수만큼 증가 연산
        res++;
    }
    res = rand() % list_h->size;	// 0 ~ (리스트 사이즈 – 1) 수 하나 반환
    
    return res;
}

/* 랜덤 수 입력 함수 */
void* worker(void *arg)
{
    WorkType work = *((WorkType *) arg); //연산되는 배열 수와 연산 반복수가 들어있는 work 변수
    int input, i;
    
    for(i=0; i<work.works; i++) {	// 연산되는 배열 수만큼 반복하여
        input = calc(work.loops);	// calc()함수에 연산 반복수를 인자로 입력하여 input 변수에 대입
        sort(input);		// input값을 sort() 함수의 인자로 입력해 호출
        //print_list();
    }
    return NULL;
}

int main(int argc, char *argv[])
{   
    pid_t process[MAX_PROCESS_SIZE];	// 매크로로 정의한 프로세스 개수만큼의 프로세스 리스트
    pid_t pid;			// 프로세스에 할당될 프로세스 ID
    int list_size, process_size, print_type;	// 리스트 크기, 프로세스 크기 변수, 출력 유형
    int i;
    WorkType args;			// 입력받은 반복수 변수
    int shm_id1, shm_id2;		// semaphore의 관련된 shmget()함수의 ID를 저장하는 변수
    key_t key = IPC_PRIVATE;		// shmget() 함수에 쓰일 설정 상수
    struct tms mytms;			// 시간 측정에 쓰일 구조체
    clock_t start_t, end_t;		// 시작 시간과 종료 시간 기록 변수
    
    /* 사용자 입력으로 프로세스 사이즈, 반복 횟수, 리스트 사이즈, 출력 유형 정의 */
    // 프로세스 사이즈: 첫 번째 인자가 있고, 첫 번째 인자로 입력받은 수가 정의한 최대 사이즈보다 작으면 MAX_PROCESS_SIZE를 사용자가 입력한 수로 재정의
    process_size = ((argc > 1) && (atoi(argv[1])<MAX_PROCESS_SIZE)) ? atoi(argv[1]): MAX_PROCESS_SIZE;
    // 반복 횟수: 두 번째 인자가 있고, 두 번째 인자로 입력받은 수가 정의한 최대 사이즈보다 작으면 MAX_LOOP_SIZE를 사용자가 입력한 수로 재정의
    args.loops = ((argc > 2) && (atoi(argv[2])<MAX_LOOP_SIZE)) ? atoi(argv[2]): MAX_LOOP_SIZE;
    // 리스트 사이즈: 세 번째 인자가 있고, 세 번째 인자로 입력받은 수가 정의한 최대 사이즈보다 작으면 MAX_LIST_SIZE를 사용자가 입력한 수로 재정의
    list_size = ((argc > 3) && (atoi(argv[3])<MAX_LIST_SIZE)) ? atoi(argv[3]): MAX_LIST_SIZE;
    // 출력 유형: 네 번째 인자가 있고, 네 번째 인자로 입력받은 수가 존재하지 않으면 0으로 재정의
    print_type = (argc > 4) ? atoi(argv[4]): 0;
    
    /* sem_open */
    // "pSem“이란 이름의 semaphore를 중복 확인 후(O_EXECL,) 생성(O_EXCL)하고,
    // 0644(-rw-r--r--)권한을 주고, 초기값을 1로 주어 생성 후, sem변수에 대입
    sem = sem_open("pSem", O_CREAT | O_EXCL, 0644, 1);
    // sem에 대입을 하였으니, "pSem“과 직접적으로 연결된 semaphore를 제거
    sem_unlink ("pSem");	// semaphore 제거
    
    /* shmget */
    // 공유 메모리를 할당받는데, 다른 방식으로 접근 못하게(IPC_PRIVATE)하고,
    // 리스트 크기만큼 할당하여, 0666(-rw-rw-rw-)권한을 부여하고, 변수에 대입
    shm_id1 = shmget(key, sizeof(int) * list_size, IPC_CREAT | 0666);
    // 위와 같이 공유 메모리를 할당받으나 ListHelper구조체 크기만큼 할당
    shm_id2 = shmget(key, sizeof(ListHelper), IPC_CREAT | 0666);
    
    /* shmat */
    // 할당받은 공유 메모리 바로 뒤에 주소 공간을 부착시키고,
    // return받은 주소값을 이용하여, 그 위치에 있는 리스트를 변수에 지정 
    shm_list = (int *) shmat(shm_id1, NULL, 0);
    // 위와 같이 공유 메모리 바로 뒤에 주소 공간을 부착시키고,
    // return받은 주소값을 이용하여, 그 위치에 있는 리스트를 변수에 지정 
    list_h = (ListHelper *) shmat(shm_id2, NULL, 0);
    
    args.works = (list_size / process_size) + 1;
    list_init(list_size);		// 리스트 초기화
    
    if ((start_t = times(&mytms)) == -1) {	// 시작 시간 에러 발생 시 에러 메시지 출력
        perror("start time error");
        exit(1);
    }
    
    // 프로세스 생성 및 조인
    for(i=0; i<process_size; i++) {
        if ((process[i] = fork()) < 0) {	// 프로세스 개수만큼 생성
            printf("fork error %d", i);
            exit(i);
        }
        else if (process[i] == 0) {
            shm_list = (int *) shmat(shm_id1, NULL, 0);		// 위 shmat 설명 참고
            list_h = (ListHelper *) shmat(shm_id2, NULL, 0);
            worker(&args.works);
            exit(0);
        }
    }
    
    for(i=0; i<process_size; i++) {	// 자식 프로세스가 모두 끝날 때까지 대기
        pid = wait(NULL);
    }
    
    if ((end_t = times(&mytms)) == -1) {	// 종료 시간 에러 발생 시 에러 메시지 출력
        perror("end time error");
        exit(1);
    }
    
    sem_unlink("pSem");	// 생성된 semaphore를 모두 닫음
    sem_close(sem);

    /* shmctl */
    // semaphore의 정보를 저장한 변수의 세그먼트를 종료/분리(IPC_RMID)시킴
    if (shmctl(shm_id1, IPC_RMID, NULL) == -1)	
    {
        perror("shmctl");
        _exit(1);
    }
    if (shmctl(shm_id2, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        _exit(1);
    }
    
    // 정렬된 리스트 출력
    if (print_type==0) {	// 출력 유형 설정을 하지 않았으면, 과제 코드 형식으로 총 발생 시간 출력
        print_list();
        printf("===================================\n");
        printf("Real time :\t\t %.4f sec\n", (double)(end_t - start_t) / CLK_TCK);
        printf("User time :\t\t %.4f sec\n", (double)mytms.tms_utime / CLK_TCK);
        printf("System time :\t\t %.4f sec\n", (double)mytms.tms_stime / CLK_TCK);
        printf("Children User time :\t %.4f sec\n", (double)mytms.tms_cutime / CLK_TCK);
        printf("Children System time :\t %.4f sec\n", (double)mytms.tms_cstime / CLK_TCK);
        printf("===================================\n");
    }
    else {			// 출력 유형 설정을 하았으면, 간추린 형식으로 총 발생 시간 출력
        printf("%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t\n", 
                (double)(end_t - start_t) / CLK_TCK,
                (double)mytms.tms_utime / CLK_TCK,
                (double)mytms.tms_stime / CLK_TCK,
                (double)mytms.tms_cutime / CLK_TCK,
                (double)mytms.tms_cstime / CLK_TCK
              );
    }
    
    return 0;
}