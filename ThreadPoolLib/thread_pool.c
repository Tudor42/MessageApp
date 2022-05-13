#include "pch.h"
#include "thread_pool.h"
#include <stdlib.h>


int CreateThreadPool(THREAD_POOL **ThreadPool, size_t NrOfThreads)
{
    if (NrOfThreads < 0 || ThreadPool == NULL) {
        return -1;
    }

    if (NrOfThreads == 0) {
        NrOfThreads = 4;
    }

    *ThreadPool = (THREAD_POOL *)malloc(sizeof(THREAD_POOL));
    if (*ThreadPool == NULL) {
        return -1;
    }

    (*ThreadPool)->threadsCount = NrOfThreads;
    (*ThreadPool)->workingCount = 0;
    (*ThreadPool)->stop = false;
    InitializeCriticalSection(&(*ThreadPool)->workMutex);
    InitializeConditionVariable(&(*ThreadPool)->workCondition);
    InitializeConditionVariable(&(*ThreadPool)->workingCondition);

    (*ThreadPool)->first = NULL;
    (*ThreadPool)->last = NULL;

    for (int i = 0; i < NrOfThreads; ++i) {
        HANDLE thread;
        thread = CreateThread(NULL, 0, ThreadPoolWorker, *ThreadPool, 0, NULL);
        if (thread != NULL) {
            CloseHandle(thread);
        }
    }
    return 0;
}

int DestroyThreadPool(THREAD_POOL **ThreadPool)
{
    PTHREAD_POOL_WORK work1, work2;

    if (ThreadPool == NULL || *ThreadPool == NULL) {
        return -1;
    }

    EnterCriticalSection(&(*ThreadPool)->workMutex);
    work1 = (*ThreadPool)->first;
    while (work1 != NULL) {
        work2 = work1->next;
        ThreadPoolWorkDestroy(work1);
        work1 = work2;
    }

    (*ThreadPool)->stop = true;
    WakeConditionVariable(&(*ThreadPool)->workCondition);
    LeaveCriticalSection(&(*ThreadPool)->workMutex);

    ThreadPoolWait(*ThreadPool);

    DeleteCriticalSection(&(*ThreadPool)->workMutex);
    return 0;
}

int ThreadPoolAddWork(THREAD_POOL *ThreadPool, THREAD_FUNC func, void *args)
{
    if (ThreadPool == NULL) {
        return -1;
    }

    PTHREAD_POOL_WORK work;

    if ((work = ThreadPoolWorkCreate(func, args)) == NULL) {
        return -1;
    }

    EnterCriticalSection(&ThreadPool->workMutex);
    if (ThreadPool->first == NULL) {
        ThreadPool->last = ThreadPool->first = work;
    }
    else {
        ThreadPool->last->next = work;
        ThreadPool->last = work;
    }
    WakeConditionVariable(&ThreadPool->workCondition);
    LeaveCriticalSection(&ThreadPool->workMutex);
    return 0;
}

void ThreadPoolWait(THREAD_POOL *ThreadPool)
{
    if (ThreadPool == NULL) {
        return;
    }
    EnterCriticalSection(&ThreadPool->workMutex);
    while (1) {
        if ((!ThreadPool->stop && ThreadPool->workingCount != 0) || (ThreadPool->stop && ThreadPool->threadsCount != 0)) {
            SleepConditionVariableCS(&ThreadPool->workingCondition, &ThreadPool->workMutex, INFINITE);
        }
        else {
            break;
        }
    }
    LeaveCriticalSection(&ThreadPool->workMutex);
}

PTHREAD_POOL_WORK ThreadPoolWorkCreate(THREAD_FUNC func, void *args)
{
    if (func == NULL) {
        return NULL;
    }

    PTHREAD_POOL_WORK work = (PTHREAD_POOL_WORK)malloc(sizeof(THREAD_POOL_WORK));
    if (work == NULL) {
        return NULL;
    }
    work->func = func;
    work->args = args;
    work->next = NULL;
    return work;
}

void ThreadPoolWorkDestroy(PTHREAD_POOL_WORK work)
{
    if (work == NULL) {
        return;
    }
    free(work);
}

PTHREAD_POOL_WORK ThreadPoolWorkGet(THREAD_POOL *ThreadPool)
{
    PTHREAD_POOL_WORK work;
    if (ThreadPool == NULL) {
        return NULL;
    }

    work = ThreadPool->first;

    if (work == NULL) {
        return NULL;
    }

    if (work->next == NULL) {
        ThreadPool->first = NULL;
        ThreadPool->last = NULL;
    }
    else {
        ThreadPool->first = work->next;
    }

    return work;
}

DWORD WINAPI ThreadPoolWorker(LPVOID args)
{
    THREAD_POOL *threadPool;
    threadPool = (THREAD_POOL *)args;

    PTHREAD_POOL_WORK work;
    while (1) {
        EnterCriticalSection(&threadPool->workMutex);
        while (threadPool->first == NULL && !threadPool->stop) {
            SleepConditionVariableCS(&threadPool->workCondition, &threadPool->workMutex, 200);
        }

        if (threadPool->stop) {
            break;
        }

        work = ThreadPoolWorkGet(threadPool);
        threadPool->workingCount++;
        LeaveCriticalSection(&threadPool->workMutex);

        if (work != (PTHREAD_POOL_WORK)NULL) {
            work->func(work->args);
            ThreadPoolWorkDestroy(work);
        }

        EnterCriticalSection(&threadPool->workMutex);
        threadPool->workingCount--;

        if (!threadPool->stop && threadPool->first == NULL && threadPool->workingCount == 0) {
            WakeConditionVariable(&threadPool->workingCondition);
        }

        LeaveCriticalSection(&threadPool->workMutex);
    }

    threadPool->threadsCount--;
    WakeConditionVariable(&threadPool->workingCondition);
    LeaveCriticalSection(&threadPool->workMutex);
    return 0;
}
