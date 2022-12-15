#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define CLK_TCK sysconf(_SC_CLK_TCK) //시간 단위 Clock Tick

#define MAX_THREAD_SIZE 100 //스레드 개수
#define MAX_LIST_SIZE 10000000 // 정렬할 배열 크기
#define MAX_LOOP_SIZE 100000000 // 연산 반복수
#define MAX_PRINT 100 // 프린트할 배열 요소 개수

typedef struct WorkType{ //WorkType 구조체
    int works; //연산되는 배열 수 
    int loops; // 연산 반복 수
} WorkType;

typedef struct ListType{ // ListTyPe 구조체
    int *list; //list 포인터변수
    int size; //- 리스트 사이즈
    int p; //- 리스트의 현재 인덱스
} ListType;

ListType list;
pthread_mutex_t mutex; //뮤텍스 객체

void list_init(int list_size) {//리스트 초기화 함수
    list.size = list_size; //리스트 사이즈
    list.list = (int *)malloc(sizeof(int) * list_size); //인자로 넘겨받은 리스트 사이즈로 설정
    list.p = 0; //초기 인덱스 값을 0으로 설정
}

void print_list() //리스트 출력 함수
{
    printf("Sorted List: [");
    for(int i=0; i<list.p; i++) {
        if (i >= MAX_PRINT) { //MAX_PRINT만큼 리스트 요소 출력
            printf("...");
            break;
        }
        printf("%d, ", list.list[i]);
    }
    printf("]\n");
}

void sort(int input) //리스트 정렬 함수
{
    int i, j, temp;//반복문에 쓰일 i,j선언 정렬에 쓰일 temp 변수 선언
    
    if (list.p>=list.size)
        return;
    
    pthread_mutex_lock(&mutex); //뮤텍스 잠금
    list.list[list.p] = input; //리스트의 현재 인덱스에 인자로 받은 input을 넣어 주고 현재 인덱스를 증가
    list.p++;
    
	
    for(i=list.p-1; i>0; i--){ //버블 정렬
        for(j=0; j<i; j++){
            if(list.list[j]>list.list[j+1]){
                temp = list.list[j];
                list.list[j] = list.list[j+1];
                list.list[j+1] = temp;
            }
        }
    }
    pthread_mutex_unlock(&mutex); //뮤텍스 잠금 해제
}

int calc(int loops) //계산 함수
{
    int res=0, i; //결과값을 담을 res, 반복문에 쓰일 i 변수 선언
    
    for (i=0; i<loops; i++) {//반복수만큼 0부터 loops값 전까지 res 증감연산
        res++;
    }
    res = rand() % list.size; //반복문에 끝난 뒤 0부터 리스트 사이즈 -1까지의 랜덤 수 하나를 res에 대입
    
    return res;//res값 리턴
}

void* worker(void *arg) //랜덤 수 입력 함수
{
    WorkType work = *((WorkType *) arg);
    int input, i; //sort()함수의 인자 변수, 반복문에 쓰일 수치 변수 i 선언
    
    for(i=0; i<work.works; i++) { //연산되는 배열 수만큼 반복하여
        input = calc(work.loops); //calc()함수에 연산 반복 수를 입력하여 input 변수에 대입
        sort(input); //input값을 sort()함수의 인자로 입력헤 호출
        //print_list();
    }
    return NULL; //NULL값 반환
}

int main(int argc, char *argv[])
{   
    pthread_t thread[MAX_THREAD_SIZE]; //매크로로 정의한 스레드 개수만큼의 스레드 리스트
    int list_size, thread_size, print_type; //리스트 크기, 스레드 크기 변수, 출력 유형
    int res, i; //반복문에 쓰일 수치 변수, 입력받은 반복수 변수
    WorkType args;
    struct tms mytms; //시간 측정에 쓰일 구조체
    clock_t start_t, end_t; //시작 시간과 종료 시간 기록 변수
    
	//사용자 입력으로 스레드 사이즈, 반복 횟수, 리스트 사이즈, 출력 유형 정의
    thread_size = ((argc > 1) && (atoi(argv[1])<MAX_THREAD_SIZE)) ? atoi(argv[1]): MAX_THREAD_SIZE;
    args.loops = ((argc > 2) && (atoi(argv[2])<MAX_LOOP_SIZE)) ? atoi(argv[2]): MAX_LOOP_SIZE;
    list_size = ((argc > 3) && (atoi(argv[3])<MAX_LIST_SIZE)) ? atoi(argv[3]): MAX_LIST_SIZE;
    print_type = (argc > 4) ? atoi(argv[4]): 0;
    
    args.works = (list_size / thread_size) + 1; //args.works 변수는 (리스트 크기 / 프로세스 개수) + 1 설정
    
    list_init(list_size); //리스트 사이즈를 넣어 list_init()함수 호출 -> 리스트 초기화
    
    if ((start_t = times(&mytms)) == -1) { //시작 시간 에러 발생 시 에러 메시지 출력
        perror("start time error");
        exit(1);
    }
    
    pthread_mutex_init(&mutex , NULL); //뮤텍스 초기화(기본 속성 사용)
    
    for(i=0; i<thread_size; i++) { //스레드 개수만큼 생성 및 조인
        res = pthread_create(&thread[i], NULL, worker, &args);
        if (res != 0) //스레드 생성 에러 발생 시 에러 메세지 출력
            perror("pthread_create error");
    }
    
    for(i=0; i<thread_size; i++) {//스레드 조인 에러 발생 시 에러 메세지 출력
        res = pthread_join(thread[i], NULL);
        if (res != 0)
            perror("pthread_join error");
    }
    
    pthread_mutex_destroy(&mutex); //뮤텍스 제거
    
    if ((end_t = times(&mytms)) == -1) { //종료 시간 에러 발생 시 에러 메세지 출력
        perror("end time error");
        exit(1);
    }
    
    if (print_type==0) {//출력 유형을 설정 하지 않았으면 ->과제 코드 형식으로 총 발생 시간 출력
        print_list(); //print_list()함수 호출 -> 정렬된 리스트 출력
        printf("===================================\n");
        printf("Real time :\t\t %.4f sec\n", (double)(end_t - start_t) / CLK_TCK);
        printf("User time :\t\t %.4f sec\n", (double)mytms.tms_utime / CLK_TCK);
        printf("System time :\t\t %.4f sec\n", (double)mytms.tms_stime / CLK_TCK);
        printf("Children User time :\t %.4f sec\n", (double)mytms.tms_cutime / CLK_TCK);
        printf("Children System time :\t %.4f sec\n", (double)mytms.tms_cstime / CLK_TCK);
        printf("===================================\n");
    }
    else {//출력 유형을 설정하였으면 -> 간추린 형식으로 총 발생 시간 출력
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