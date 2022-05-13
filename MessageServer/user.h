#pragma once

#include "server_communication_api.h"
#include "thread_pool.h"

typedef struct _USER {
    CM_SERVER_CLIENT *client;
    THREAD_POOL *threadPool;
    char *userName;
} USER, *PUSER;

struct {
    DWORD capacity;
    DWORD size;
    SRWLOCK mutex;
    PUSER users;
} gUsersVector;

int InitUsersVector(DWORD InitCapacity);

void UninitUsersVector();

int UserLoggedName(const char *user);

int UserLoggedSocket(CM_SERVER_CLIENT *user);

int AddUser(CM_SERVER_CLIENT *User, const char *UserName, THREAD_POOL *ThreadPool);

int RemoveUser(CM_SERVER_CLIENT *User);