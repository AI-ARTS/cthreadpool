
#include"threadpoll.h"


void taskFunc(void* arg) {
    int num = *(int*)arg;
    printf("thread %ld is working, number=%d\n",pthread_self(), num);
    usleep(500);
}


int main()
{
    // 创建线程池
    threadpool* pool = ThreadPool(3, 10, 100);

    if (pool == NULL) {
        printf("[error]pool build faild...\n");
        return 0;
    }
    // 进行任务的添加，就是添加到队列中
    for (int i = 0; i < 100; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;
        threadAddWork(pool, taskFunc, num);
    }
    printf("[info] submit work over...\n");
    sleep(3);
    threadpoolDestroy(pool);
    printf("thread pool working over...\n");
    return 0;
}

