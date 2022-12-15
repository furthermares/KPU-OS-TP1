#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define CLK_TCK sysconf(_SC_CLK_TCK) //�ð� ���� Clock Tick

#define MAX_THREAD_SIZE 100 //������ ����
#define MAX_LIST_SIZE 10000000 // ������ �迭 ũ��
#define MAX_LOOP_SIZE 100000000 // ���� �ݺ���
#define MAX_PRINT 100 // ����Ʈ�� �迭 ��� ����

typedef struct WorkType{ //WorkType ����ü
    int works; //����Ǵ� �迭 �� 
    int loops; // ���� �ݺ� ��
} WorkType;

typedef struct ListType{ // ListTyPe ����ü
    int *list; //list �����ͺ���
    int size; //- ����Ʈ ������
    int p; //- ����Ʈ�� ���� �ε���
} ListType;

ListType list;
pthread_mutex_t mutex; //���ؽ� ��ü

void list_init(int list_size) {//����Ʈ �ʱ�ȭ �Լ�
    list.size = list_size; //����Ʈ ������
    list.list = (int *)malloc(sizeof(int) * list_size); //���ڷ� �Ѱܹ��� ����Ʈ ������� ����
    list.p = 0; //�ʱ� �ε��� ���� 0���� ����
}

void print_list() //����Ʈ ��� �Լ�
{
    printf("Sorted List: [");
    for(int i=0; i<list.p; i++) {
        if (i >= MAX_PRINT) { //MAX_PRINT��ŭ ����Ʈ ��� ���
            printf("...");
            break;
        }
        printf("%d, ", list.list[i]);
    }
    printf("]\n");
}

void sort(int input) //����Ʈ ���� �Լ�
{
    int i, j, temp;//�ݺ����� ���� i,j���� ���Ŀ� ���� temp ���� ����
    
    if (list.p>=list.size)
        return;
    
    pthread_mutex_lock(&mutex); //���ؽ� ���
    list.list[list.p] = input; //����Ʈ�� ���� �ε����� ���ڷ� ���� input�� �־� �ְ� ���� �ε����� ����
    list.p++;
    
	
    for(i=list.p-1; i>0; i--){ //���� ����
        for(j=0; j<i; j++){
            if(list.list[j]>list.list[j+1]){
                temp = list.list[j];
                list.list[j] = list.list[j+1];
                list.list[j+1] = temp;
            }
        }
    }
    pthread_mutex_unlock(&mutex); //���ؽ� ��� ����
}

int calc(int loops) //��� �Լ�
{
    int res=0, i; //������� ���� res, �ݺ����� ���� i ���� ����
    
    for (i=0; i<loops; i++) {//�ݺ�����ŭ 0���� loops�� ������ res ��������
        res++;
    }
    res = rand() % list.size; //�ݺ����� ���� �� 0���� ����Ʈ ������ -1������ ���� �� �ϳ��� res�� ����
    
    return res;//res�� ����
}

void* worker(void *arg) //���� �� �Է� �Լ�
{
    WorkType work = *((WorkType *) arg);
    int input, i; //sort()�Լ��� ���� ����, �ݺ����� ���� ��ġ ���� i ����
    
    for(i=0; i<work.works; i++) { //����Ǵ� �迭 ����ŭ �ݺ��Ͽ�
        input = calc(work.loops); //calc()�Լ��� ���� �ݺ� ���� �Է��Ͽ� input ������ ����
        sort(input); //input���� sort()�Լ��� ���ڷ� �Է��� ȣ��
        //print_list();
    }
    return NULL; //NULL�� ��ȯ
}

int main(int argc, char *argv[])
{   
    pthread_t thread[MAX_THREAD_SIZE]; //��ũ�η� ������ ������ ������ŭ�� ������ ����Ʈ
    int list_size, thread_size, print_type; //����Ʈ ũ��, ������ ũ�� ����, ��� ����
    int res, i; //�ݺ����� ���� ��ġ ����, �Է¹��� �ݺ��� ����
    WorkType args;
    struct tms mytms; //�ð� ������ ���� ����ü
    clock_t start_t, end_t; //���� �ð��� ���� �ð� ��� ����
    
	//����� �Է����� ������ ������, �ݺ� Ƚ��, ����Ʈ ������, ��� ���� ����
    thread_size = ((argc > 1) && (atoi(argv[1])<MAX_THREAD_SIZE)) ? atoi(argv[1]): MAX_THREAD_SIZE;
    args.loops = ((argc > 2) && (atoi(argv[2])<MAX_LOOP_SIZE)) ? atoi(argv[2]): MAX_LOOP_SIZE;
    list_size = ((argc > 3) && (atoi(argv[3])<MAX_LIST_SIZE)) ? atoi(argv[3]): MAX_LIST_SIZE;
    print_type = (argc > 4) ? atoi(argv[4]): 0;
    
    args.works = (list_size / thread_size) + 1; //args.works ������ (����Ʈ ũ�� / ���μ��� ����) + 1 ����
    
    list_init(list_size); //����Ʈ ����� �־� list_init()�Լ� ȣ�� -> ����Ʈ �ʱ�ȭ
    
    if ((start_t = times(&mytms)) == -1) { //���� �ð� ���� �߻� �� ���� �޽��� ���
        perror("start time error");
        exit(1);
    }
    
    pthread_mutex_init(&mutex , NULL); //���ؽ� �ʱ�ȭ(�⺻ �Ӽ� ���)
    
    for(i=0; i<thread_size; i++) { //������ ������ŭ ���� �� ����
        res = pthread_create(&thread[i], NULL, worker, &args);
        if (res != 0) //������ ���� ���� �߻� �� ���� �޼��� ���
            perror("pthread_create error");
    }
    
    for(i=0; i<thread_size; i++) {//������ ���� ���� �߻� �� ���� �޼��� ���
        res = pthread_join(thread[i], NULL);
        if (res != 0)
            perror("pthread_join error");
    }
    
    pthread_mutex_destroy(&mutex); //���ؽ� ����
    
    if ((end_t = times(&mytms)) == -1) { //���� �ð� ���� �߻� �� ���� �޼��� ���
        perror("end time error");
        exit(1);
    }
    
    if (print_type==0) {//��� ������ ���� ���� �ʾ����� ->���� �ڵ� �������� �� �߻� �ð� ���
        print_list(); //print_list()�Լ� ȣ�� -> ���ĵ� ����Ʈ ���
        printf("===================================\n");
        printf("Real time :\t\t %.4f sec\n", (double)(end_t - start_t) / CLK_TCK);
        printf("User time :\t\t %.4f sec\n", (double)mytms.tms_utime / CLK_TCK);
        printf("System time :\t\t %.4f sec\n", (double)mytms.tms_stime / CLK_TCK);
        printf("Children User time :\t %.4f sec\n", (double)mytms.tms_cutime / CLK_TCK);
        printf("Children System time :\t %.4f sec\n", (double)mytms.tms_cstime / CLK_TCK);
        printf("===================================\n");
    }
    else {//��� ������ �����Ͽ����� -> ���߸� �������� �� �߻� �ð� ���
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