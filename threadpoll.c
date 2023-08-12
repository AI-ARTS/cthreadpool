#include "threadpoll.h"


// 调用初始化线程池
threadpool* ThreadPool(int minNum, int maxNum, int capacity)
{	
	/*
		创建线程池threadpool指针且开辟空间
		参数初始化
		任务队列开辟空间，且，空任务对象，flag  = -1
		工人线程、管理者线程、锁创建和初始化    // 统一空位置为-1
		工人线程创建的threadid保存空间没有threadID时候，赋值为1
	*/
	threadpool* pool = NULL;
	do
	{
		// 创建pool 对象
		pool = (threadpool*)malloc(sizeof(threadpool));

		if (pool == NULL) {
			printf("[info]pool malloc faild...\n");
			break;
		}

		// 线程池的大小进行
		pool->minNum = minNum;
		pool->maxNum = maxNum;


		// 针对池中线程的状态的记录
		pool->liveNum = 0;
		pool->exitNum = 0;
		pool->busyNum = 0;


		// 进行任务队列的初始化
		pool->taskqueue = (task*)malloc(sizeof(task) * capacity);
		if (pool->taskqueue == NULL) {
			printf("[info]create taskqueue faild...\n");
			break;
		}
		// 任务队列初始化
		for (int i = 0; i < capacity; i++) pool->taskqueue[i].flag = -1;


		// 任务队列参数初始化
		pool->taskCapacity = capacity; // 任务空间大小
		pool->taskSize = 0;              
		pool->shutdown = 0;


		// 锁的初始化
		if (pthread_mutex_init(&pool->threadpool_metux, NULL) !=0 ||
			pthread_mutex_init(&pool->busy_metux, NULL)!=0  ||
			pthread_cond_init(&pool->notFull, NULL)!=0 ||
			pthread_cond_init(&pool->notEmpty, NULL)!=0) {
			printf("[info]create mutex faild.\n");
			break;
		}
		printf("[info]create mutex successful.\n");


		// 管理者线程
		if (pthread_create(&pool->manage, NULL, managerFun, pool) != 0) {
			printf("[info]create manager pthread faild.\n");
			break;
		}
		printf("[info]create manager pthread successful\n");


		// 工人线程的初始化
		pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * maxNum); // 首先创建最大的线程数量空间
		printf("[info]create list threadIDs  successful.\n");


		// 初始化工人线程空间
		for (int i = 0; i < maxNum; i++) pool->threadIDs[i] == -1;
		if (!pool->threadIDs) break;


		// 创建工人线程
		for (int i = 0; i < minNum; i++) {
			pool->liveNum++;
			if (pthread_create(&pool->threadIDs[i], NULL, worker, pool) != 0) {
				printf("[info]create worker faild.\n");
				break;
			}
		}
		// 初始化完毕
		printf("[info]create pool successful.\n");
		return pool;

	} while (0);
	

	// 如果创建失败，进行工人空间的销毁
	if (pool && pool->threadIDs)free(pool->threadIDs);
	// 进行任务队列的销毁
	if (pool && pool->taskqueue) free(pool->taskqueue);
	// 进行线程池的销毁
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
	// 自身的事情结束了，全部退出
	pthread_mutex_unlock(&pool->threadpool_metux);
	pthread_exit(NULL);
}

void* worker(void* arg)
{
	// 线程运行worker，从pool中拿到任务
	// 进行线程池函数的转换
	threadpool* pool = (threadpool*)arg;

	while (1) { // 不停的去进行数据的操作， 需要操作线程池

		pthread_mutex_lock(&pool->threadpool_metux); // 抢占cup时间片去进行worker的工作

		// 当前任务队列是否为空

		while (pool->taskSize == 0 && !pool->shutdown) {
			// 任务为空，但是线程池没死， 进行阻塞工作的线程
			pthread_cond_wait(&pool->notEmpty, &pool->threadpool_metux);

			if (pool->exitNum > 0) {
				pool->exitNum--;
				if (pool->liveNum > pool->minNum) {
					// 防止。
					threadexit(pool);
				}
				// 线程退出！
			}
		}

		// 判断线程池是否被关闭了
		if (pool->shutdown) {
			threadexit(pool);
		}
		// 从任务队列中取出一个任务
		task taskfun;
		// 循环取出 取任务
		int i = 0;
		for (i = 0; i < pool->taskCapacity; i++) {
			if ((pool->taskqueue)[i].flag!=-1) {
				// 将任务取出，就是复制一份，然后进行计算，那么将pool队列中的任务的标志设置为-1；
				taskfun = (pool->taskqueue)[i]; 
				// 拿到任务之后，开始打扫任务空间
				(pool->taskqueue)[i].flag = -1; 
				(pool->taskqueue)[i].function = NULL;
				(pool->taskqueue)[i].arg = NULL;
				// 任务参数的调整
				pool->taskSize--;
				if(pool->taskSize == pool->taskCapacity-1) pthread_cond_signal(&pool->notFull);

				pthread_mutex_unlock(&pool->threadpool_metux);
				break;
			}

		}
		

		// 开始运行，现在就加上busy的锁
		pthread_mutex_lock(&pool->busy_metux);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->busy_metux);


		//去执行这个函数
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

	// 创建线程和销毁线程。
	while (!pool->shutdown) {
		sleep(3);

		// 取出线程池中任务数量和当前线程的数量
		pthread_mutex_lock(&pool->busy_metux);
		// 拿到目前任务的数量
		int queuesize = pool->taskSize;
		// 拿到目前存活的线程的数量
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->busy_metux);



		// 取出忙线程
		pthread_mutex_lock(&pool->busy_metux);
		int busy = pool->busyNum;
		pthread_mutex_unlock(&pool->busy_metux);


		// 添加线程
		if (queuesize > liveNum && liveNum < pool->maxNum) {
			// 寻找空房间// 每次增加两个
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
		// 销毁线程
		// 规则，忙的线程*2 < 存活的线程数 && 存活的线程数 > 最小线程数
		if (busy * 2 < liveNum && liveNum > pool->minNum) {
			// 进行线程的销毁。
			pthread_mutex_lock(&pool->threadpool_metux);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->threadpool_metux);
			// 让线程自杀
			for (int i = 0; i < NUMBER; i++) {
				pthread_cond_signal(&pool->notEmpty); // 在这个变量上发送唤醒信号
			}
		}
	}
	return NULL;
}

void threadAddWork(threadpool* pool, void(*func)(void*), void* arg) {

	// 进行线程池的加锁
	pthread_mutex_lock(&pool->threadpool_metux);

	while (pool->taskSize == pool->taskCapacity && !pool->shutdown) {
		// 阻塞生产线程
		pthread_cond_wait(&pool->notFull, &pool->threadpool_metux);
	}

	if (pool->shutdown) {
		pthread_mutex_unlock(&pool->threadpool_metux);
		return;
	}

	// 添加任务
	for (int i = 0; i < pool->taskCapacity; i++) {
		if (pool->taskqueue[i].flag == -1) {
			// 找到了进行添加任务
			pool->taskqueue[i].flag = 1;
			pool->taskqueue[i].arg = arg;
			pool->taskqueue[i].function = func;
			pool->taskSize++;
			printf("[info] add work successful.\n");
			break;
		}
	}
	// 唤醒阻塞线程，现在有任务了
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

	// 关闭线程池
	pool->shutdown = 1;
	// 阻塞回收管理者线程
	printf("[info]waiting manage thread finished...\n");
	pthread_join(pool->manage, NULL);
	printf("[info]manage thread finished...\n");

	// 唤醒阻塞的消费者线程
	
	for (int i = 0; i < pool->liveNum; i++) {
		pthread_cond_signal(&pool->notEmpty);
	}
	printf("[info]work thread finished...\n");

	pthread_mutex_destroy(&pool->threadpool_metux);
	pthread_mutex_destroy(&pool->busy_metux);
	pthread_cond_destroy(&pool->notFull);
	pthread_cond_destroy(&pool->notEmpty);
	// 释放掉堆内存
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
