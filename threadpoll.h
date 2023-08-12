
// 进行c线程池的架构
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include<time.h>
#define NUMBER 2

// 进行任务的声明
typedef struct task {
	void(* function)(void* arg); // 需要运行的函数的指针
	void* arg; // 参数指针
	int flag; // -1表示不可用 1表示可用
}task;

// 进行线程池的声明
typedef struct threadpool {
	
	int maxNum;	// 线程最大数量
	int minNum; // 线程的最小数量
	int liveNum; // 存活的线程数量
	int exitNum; // 需要退出的线程的数量
	int busyNum; // 忙中的线程的数量

	// 现在进行任务的的数据的构建
	struct task* taskqueue;

	int taskCapacity; // 也就是任务队列的容量
	int taskSize; // 此时任务队列中的任务是多少


	pthread_t manage; // 管理者ID
	pthread_t* threadIDs; // 打工者线程


	// 进行全局锁的构建
	pthread_mutex_t threadpool_metux;
	// busy的锁，我现在不太清除是否要进行这个锁的构建
	pthread_mutex_t busy_metux;

	// 需要记性两个条件变量的构建， 任务队列是不是满了，任务队列是不是空了
	pthread_cond_t notFull;
	pthread_cond_t notEmpty;
	int shutdown;  // 1销毁。0不销毁

}threadpool;


// 线程池的初始化
struct threadpool* ThreadPool(int minNum, int maxNum, int capacity);

void* worker(void *arg);
void* managerFun(void* arg);
void threadexit(threadpool* pool);
void threadAddWork(threadpool* pool, void(*func)(void*), void* arg);
int threadbusy(threadpool* pool);
int threadlive(threadpool* pool);
int threadpoolDestroy(threadpool* pool);
