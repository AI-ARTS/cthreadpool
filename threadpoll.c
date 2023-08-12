#include "threadpoll.h"


// ���ó�ʼ���̳߳�
threadpool* ThreadPool(int minNum, int maxNum, int capacity)
{	
	/*
		�����̳߳�threadpoolָ���ҿ��ٿռ�
		������ʼ��
		������п��ٿռ䣬�ң����������flag  = -1
		�����̡߳��������̡߳��������ͳ�ʼ��    // ͳһ��λ��Ϊ-1
		�����̴߳�����threadid����ռ�û��threadIDʱ�򣬸�ֵΪ1
	*/
	threadpool* pool = NULL;
	do
	{
		// ����pool ����
		pool = (threadpool*)malloc(sizeof(threadpool));

		if (pool == NULL) {
			printf("[info]pool malloc faild...\n");
			break;
		}

		// �̳߳صĴ�С����
		pool->minNum = minNum;
		pool->maxNum = maxNum;


		// ��Գ����̵߳�״̬�ļ�¼
		pool->liveNum = 0;
		pool->exitNum = 0;
		pool->busyNum = 0;


		// ����������еĳ�ʼ��
		pool->taskqueue = (task*)malloc(sizeof(task) * capacity);
		if (pool->taskqueue == NULL) {
			printf("[info]create taskqueue faild...\n");
			break;
		}
		// ������г�ʼ��
		for (int i = 0; i < capacity; i++) pool->taskqueue[i].flag = -1;


		// ������в�����ʼ��
		pool->taskCapacity = capacity; // ����ռ��С
		pool->taskSize = 0;              
		pool->shutdown = 0;


		// ���ĳ�ʼ��
		if (pthread_mutex_init(&pool->threadpool_metux, NULL) !=0 ||
			pthread_mutex_init(&pool->busy_metux, NULL)!=0  ||
			pthread_cond_init(&pool->notFull, NULL)!=0 ||
			pthread_cond_init(&pool->notEmpty, NULL)!=0) {
			printf("[info]create mutex faild.\n");
			break;
		}
		printf("[info]create mutex successful.\n");


		// �������߳�
		if (pthread_create(&pool->manage, NULL, managerFun, pool) != 0) {
			printf("[info]create manager pthread faild.\n");
			break;
		}
		printf("[info]create manager pthread successful\n");


		// �����̵߳ĳ�ʼ��
		pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * maxNum); // ���ȴ��������߳������ռ�
		printf("[info]create list threadIDs  successful.\n");


		// ��ʼ�������߳̿ռ�
		for (int i = 0; i < maxNum; i++) pool->threadIDs[i] == -1;
		if (!pool->threadIDs) break;


		// ���������߳�
		for (int i = 0; i < minNum; i++) {
			pool->liveNum++;
			if (pthread_create(&pool->threadIDs[i], NULL, worker, pool) != 0) {
				printf("[info]create worker faild.\n");
				break;
			}
		}
		// ��ʼ�����
		printf("[info]create pool successful.\n");
		return pool;

	} while (0);
	

	// �������ʧ�ܣ����й��˿ռ������
	if (pool && pool->threadIDs)free(pool->threadIDs);
	// ����������е�����
	if (pool && pool->taskqueue) free(pool->taskqueue);
	// �����̳߳ص�����
	if (pool) free(pool);

	return NULL;

}

void threadexit(threadpool* pool) {
	for (int i = 0; i < pool->maxNum; i++) {
		if (pthread_self() == pool->threadIDs[i]) {
			pool->threadIDs[i] = -1;
			pool->liveNum--;
			printf("thread %ld exit...\n", pthread_self());
			break;
		}
	}
	// �������������ˣ�ȫ���˳�
	pthread_mutex_unlock(&pool->threadpool_metux);
	pthread_exit(NULL);
}

void* worker(void* arg)
{
	// �߳�����worker����pool���õ�����
	// �����̳߳غ�����ת��
	threadpool* pool = (threadpool*)arg;

	while (1) { // ��ͣ��ȥ�������ݵĲ����� ��Ҫ�����̳߳�

		pthread_mutex_lock(&pool->threadpool_metux); // ��ռcupʱ��Ƭȥ����worker�Ĺ���

		// ��ǰ��������Ƿ�Ϊ��

		while (pool->taskSize == 0 && !pool->shutdown) {
			// ����Ϊ�գ������̳߳�û���� ���������������߳�
			pthread_cond_wait(&pool->notEmpty, &pool->threadpool_metux);

			if (pool->exitNum > 0) {
				pool->exitNum--;
				if (pool->liveNum > pool->minNum) {
					// ��ֹ��
					threadexit(pool);
				}
				// �߳��˳���
			}
		}

		// �ж��̳߳��Ƿ񱻹ر���
		if (pool->shutdown) {
			threadexit(pool);
		}
		// �����������ȡ��һ������
		task taskfun;
		// ѭ��ȡ�� ȡ����
		int i = 0;
		for (i = 0; i < pool->taskCapacity; i++) {
			if ((pool->taskqueue)[i].flag!=-1) {
				// ������ȡ�������Ǹ���һ�ݣ�Ȼ����м��㣬��ô��pool�����е�����ı�־����Ϊ-1��
				taskfun = (pool->taskqueue)[i]; 
				// �õ�����֮�󣬿�ʼ��ɨ����ռ�
				(pool->taskqueue)[i].flag = -1; 
				(pool->taskqueue)[i].function = NULL;
				(pool->taskqueue)[i].arg = NULL;
				// ��������ĵ���
				pool->taskSize--;
				if(pool->taskSize == pool->taskCapacity-1) pthread_cond_signal(&pool->notFull);

				pthread_mutex_unlock(&pool->threadpool_metux);
				break;
			}

		}
		

		// ��ʼ���У����ھͼ���busy����
		pthread_mutex_lock(&pool->busy_metux);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->busy_metux);


		//ȥִ���������
		taskfun.function(taskfun.arg);
		free(taskfun.arg);
		taskfun.arg = NULL;

		pthread_mutex_lock(&pool->busy_metux);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->busy_metux);
	}
	return NULL;
}

void* managerFun(void* arg)
{
	threadpool* pool = (threadpool*) arg;

	// �����̺߳������̡߳�
	while (!pool->shutdown) {
		sleep(3);

		// ȡ���̳߳������������͵�ǰ�̵߳�����
		pthread_mutex_lock(&pool->busy_metux);
		// �õ�Ŀǰ���������
		int queuesize = pool->taskSize;
		// �õ�Ŀǰ�����̵߳�����
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->busy_metux);



		// ȡ��æ�߳�
		pthread_mutex_lock(&pool->busy_metux);
		int busy = pool->busyNum;
		pthread_mutex_unlock(&pool->busy_metux);


		// ����߳�
		if (queuesize > liveNum && liveNum < pool->maxNum) {
			// Ѱ�ҿշ���// ÿ����������
			int counter = 0;
			int i = 0;
			pthread_mutex_lock(&pool->threadpool_metux);

			for (i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum<pool->maxNum; i++) {
				if (pool->threadIDs[i] == -1) {
					counter++;
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					pool->liveNum++;	
				}
			}
			pthread_mutex_unlock(&pool->threadpool_metux);
		}
		// �����߳�
		// ����æ���߳�*2 < �����߳��� && �����߳��� > ��С�߳���
		if (busy * 2 < liveNum && liveNum > pool->minNum) {
			// �����̵߳����١�
			pthread_mutex_lock(&pool->threadpool_metux);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->threadpool_metux);
			// ���߳���ɱ
			for (int i = 0; i < NUMBER; i++) {
				pthread_cond_signal(&pool->notEmpty); // ����������Ϸ��ͻ����ź�
			}
		}
	}
	return NULL;
}

void threadAddWork(threadpool* pool, void(*func)(void*), void* arg) {

	// �����̳߳صļ���
	pthread_mutex_lock(&pool->threadpool_metux);

	while (pool->taskSize == pool->taskCapacity && !pool->shutdown) {
		// ���������߳�
		pthread_cond_wait(&pool->notFull, &pool->threadpool_metux);
	}

	if (pool->shutdown) {
		pthread_mutex_unlock(&pool->threadpool_metux);
		return;
	}

	// �������
	for (int i = 0; i < pool->taskCapacity; i++) {
		if (pool->taskqueue[i].flag == -1) {
			// �ҵ��˽����������
			pool->taskqueue[i].flag = 1;
			pool->taskqueue[i].arg = arg;
			pool->taskqueue[i].function = func;
			pool->taskSize++;
			printf("[info] add work successful.\n");
			break;
		}
	}
	// ���������̣߳�������������
	pthread_cond_signal(&pool->notEmpty);
	pthread_mutex_unlock(&pool->threadpool_metux);

}

int threadbusy(threadpool* pool)
{
	int ans;
	pthread_mutex_lock(&pool->threadpool_metux);
	ans = pool->busyNum;
	pthread_mutex_unlock(&pool->threadpool_metux);
	return ans;
}

int threadpoolDestroy(threadpool* pool) {
	if (pool == NULL) return -1;

	// �ر��̳߳�
	pool->shutdown = 1;
	// �������չ������߳�
	printf("[info]waiting manage thread finished...\n");
	pthread_join(pool->manage, NULL);
	printf("[info]manage thread finished...\n");

	// �����������������߳�
	
	for (int i = 0; i < pool->liveNum; i++) {
		pthread_cond_signal(&pool->notEmpty);
	}
	printf("[info]work thread finished...\n");

	pthread_mutex_destroy(&pool->threadpool_metux);
	pthread_mutex_destroy(&pool->busy_metux);
	pthread_cond_destroy(&pool->notFull);
	pthread_cond_destroy(&pool->notEmpty);
	// �ͷŵ����ڴ�
	if (pool->threadIDs) {
		free(pool->threadIDs);
		pool->threadIDs = NULL;
		printf("[info]thread queue release..\n");
	}
	if (pool->taskqueue) {
		free(pool->taskqueue);
		pool->taskqueue = NULL;
		printf("[info]task queue release..\n");
	}

	printf("[info]mutex and cond destory...\n");
	free(pool);
	printf("[info]pool destory...\n");
	pool = NULL;
}
