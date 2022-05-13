// MessageClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
// communication library
#include "communication_api.h"
// thread pool library
#include "thread_pool.h"

#include "thread_functions.h"

int _tmain(int argc, TCHAR *argv[])
{
    /*
        This main implementation can be used as an initial example.
        You can erase main implementation when is no longer helpful.
    */

    (void)argc;
    (void)argv;

    EnableCommunicationModuleLogger();

    CM_ERROR error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
        return -1;
    }
    
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), !ENABLE_ECHO_INPUT);
    InitThreadFunctions();

    CM_CLIENT *client = NULL;
    CM_DATA_BUFFER *sendBuffer = NULL;
    CM_SIZE sendSize;
    error = CreateDataBuffer(&sendBuffer, (CM_SIZE)32);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("DataBuffer failed with err-code=0x%X!\n"), error);
        UninitCommunicationModule();

        return -1;
    }

    CM_DATA_BUFFER *receiveBuffer = NULL;
    CM_SIZE receiveSize;
    error = CreateDataBuffer(&receiveBuffer, (CM_SIZE)32);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("DataBuffer failed with err-code=0x%X!\n"), error);
        UninitCommunicationModule();
        return -1;
    }

    error = CreateClientConnectionToServer(&client);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("CreateClientConnectionToServer failed with err-code=0x%X!\n"), error);
        UninitCommunicationModule();
        goto finish;
    }

    CopyDataIntoBuffer(sendBuffer, (const CM_BYTE *)"check", sizeof("check"));
    error = SendDataToServer(client, sendBuffer, &sendSize);

    if (sendSize != sizeof("check") || CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("Error: maximum concurrent connection count reached\n"));
        UninitCommunicationModule();
        return -1;
    }

    error = ReceiveDataFormServer(client, receiveBuffer, &receiveSize);

    if (strcmp((char *)receiveBuffer->DataBuffer, "accept") == 0) {
        _tprintf_s(TEXT("We are connected to the server...\n"));
    }
    else if (strcmp((char *)receiveBuffer->DataBuffer, "reject") == 0) {
        _tprintf_s(TEXT("Error: server is full\n"));
        UninitCommunicationModule();
        DestroyDataBuffer(sendBuffer);
        DestroyDataBuffer(receiveBuffer);
        return -1;
    }
    else {
        _tprintf_s(TEXT("Can't connect to server\n"));
        UninitCommunicationModule();
        DestroyDataBuffer(sendBuffer);
        DestroyDataBuffer(receiveBuffer);
        return -1;
    }


    THREAD_POOL *threadPool;
    void *arg = client;
    CreateThreadPool(&threadPool, 6);
    ThreadPoolAddWork(threadPool, StartConsole, (void *)arg);
    ThreadPoolAddWork(threadPool, GetDataServer, (void *)arg);
    ThreadPoolWait(threadPool);
    while (1);

finish:
    _tprintf_s(TEXT("Client is shutting down now...\n"));
    UninitThreadFunctions();
    DestroyClient(client);
    UninitCommunicationModule();

    return 0;
}

