#pragma once
#include "server_communication_api.h"
#include "thread_pool.h"

struct {
    LONG nr;
    CRITICAL_SECTION mutex;
} gConnectedUsers;

HANDLE gRegistrationFile;
SRWLOCK gRegistrationFileMutex;

void IncrementConnectedUsers();
void DecrementConnectedUsers();

int InitThreadFunctions();
void UninitThreadFunctions();

void StartConnection(void *arg);
