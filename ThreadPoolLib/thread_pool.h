#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <stdbool.h>
#include <windows.h>
#include <stdlib.h>

typedef void (*THREAD_FUNC)(void *arg);


typedef struct _THREAD_POOL_WORK {
    THREAD_FUNC func;
    void *args;
    struct _THREAD_POOL_WORK *next;
} THREAD_POOL_WORK, *PTHREAD_POOL_WORK;

typedef struct {
    PTHREAD_POOL_WORK first;
    PTHREAD_POOL_WORK last;
    CRITICAL_SECTION workMutex;
    CONDITION_VARIABLE workCondition;
    CONDITION_VARIABLE workingCondition;
    size_t workingCount; // nr of threads working
    size_t threadsCount; // total nr of threads
    bool stop;
} THREAD_POOL;

int CreateThreadPool(THREAD_POOL **ThreadPool, size_t NrOfThreads);
int DestroyThreadPool(THREAD_POOL **ThreadPool);

int ThreadPoolAddWork(THREAD_POOL *ThreadPool, THREAD_FUNC func, void *args);
void ThreadPoolWait(THREAD_POOL *ThreadPool);

PTHREAD_POOL_WORK ThreadPoolWorkCreate(THREAD_FUNC func, void *args);
void ThreadPoolWorkDestroy(PTHREAD_POOL_WORK work);

PTHREAD_POOL_WORK ThreadPoolWorkGet(THREAD_POOL *ThreadPool);

DWORD WINAPI ThreadPoolWorker(LPVOID args);
#endif