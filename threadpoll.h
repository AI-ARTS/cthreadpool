
// ����c�̳߳صļܹ�
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include<time.h>
#define NUMBER 2

// �������������
typedef struct task {
	void(* function)(void* arg); // ��Ҫ���еĺ�����ָ��
	void* arg; // ����ָ��
	int flag; // -1��ʾ������ 1��ʾ����
}task;

// �����̳߳ص�����
typedef struct threadpool {
	
	int maxNum;	// �߳��������
	int minNum; // �̵߳���С����
	int liveNum; // �����߳�����
	int exitNum; // ��Ҫ�˳����̵߳�����
	int busyNum; // æ�е��̵߳�����

	// ���ڽ�������ĵ����ݵĹ���
	struct task* taskqueue;

	int taskCapacity; // Ҳ����������е�����
	int taskSize; // ��ʱ��������е������Ƕ���


	pthread_t manage; // ������ID
	pthread_t* threadIDs; // �����߳�


	// ����ȫ�����Ĺ���
	pthread_mutex_t threadpool_metux;
	// busy�����������ڲ�̫����Ƿ�Ҫ����������Ĺ���
	pthread_mutex_t busy_metux;

	// ��Ҫ�����������������Ĺ����� ��������ǲ������ˣ���������ǲ��ǿ���
	pthread_cond_t notFull;
	pthread_cond_t notEmpty;
	int shutdown;  // 1���١�0������

}threadpool;


// �̳߳صĳ�ʼ��
struct threadpool* ThreadPool(int minNum, int maxNum, int capacity);

void* worker(void *arg);
void* managerFun(void* arg);
void threadexit(threadpool* pool);
void threadAddWork(threadpool* pool, void(*func)(void*), void* arg);
int threadbusy(threadpool* pool);
int threadlive(threadpool* pool);
int threadpoolDestroy(threadpool* pool);
