#pragma once
#include "thread_pool.h"
#include "client_communication_api.h"
#include <WinBase.h>

typedef struct _CONTROL_VAR {
    BOOL stop;
    CRITICAL_SECTION mutex;
}CONTROL_VAR;

CONTROL_VAR gControlVar;

typedef struct _IO_BUFF {
    char *buff;
    DWORD size;
    DWORD capacity;
    CRITICAL_SECTION mutex;
}READ_BUFF;
READ_BUFF gReadBuff;


void InitThreadFunctions();

void UninitThreadFunctions();


void InitReadWriteBuff();

void UninitReadWriteBuff();

void ReadConsoleThread();

void WriteBeforeReadBuffer(const char *Data, DWORD DataSize);

void SendReadBuffer(CM_CLIENT *client);

void StartConsole(void *args);

void Parse(char *str, CM_SIZE size, char **words, int *nrWords);

void GetDataServer(void *args);