// MessageServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
// communication library
#include "communication_api.h"
// thread pool library
#include "thread_pool.h"
#include "user.h"

int _tmain(int argc, TCHAR* argv[])
{
    /*
    This main implementation can be used as an initial example.
    You can erase main implementation when is no longer helpful.
    */

    (void)argc;
    (void)argv;
    if (argc < 2) {
        _tprintf_s(TEXT("Error: maximum number of connections missing!\n"));
        return -1;
    }

    if (argc > 2) {
        _tprintf_s(TEXT("Error: too many arguments!\n"));
        return -1;
    }

    LONG maxNumberOfUsers = 0;

    __try {
        maxNumberOfUsers = _tstoi(argv[1]);
    }
    __finally {
        if (maxNumberOfUsers <= 0) {
            _tprintf_s(TEXT("Error: invalid maximum number of connections!\n"));
            return -1;
        }
    }

    EnableCommunicationModuleLogger();

    CM_ERROR error = InitCommunicationModule();
    if (-1 == InitThreadFunctions())
        return -1;
    InitUsersVector(maxNumberOfUsers);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
        return -1;
    }

    
    CM_SERVER* server = NULL;
    error = CreateServer(&server);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("CreateServer failed with err-code=0x%X!\n"), error);
        UninitCommunicationModule();
        return -1;
    }

    THREAD_POOL *threadPool = NULL;
    if (CreateThreadPool(&threadPool, 4) == -1) {
        _tprintf_s(TEXT("Error: CreateThreadPool failed"));
        UninitCommunicationModule();
        return -1;
    }

    _tprintf_s(TEXT("Server is Up & Running...\n"));    

    CM_SERVER_CLIENT* newClient;
    CM_DATA_BUFFER *sendBuffer, *receiveBuffer;
    error = CreateDataBuffer(&sendBuffer, 7);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("Error: DataBuffer creation failed\n"));
        UninitCommunicationModule();
        return -1;
    }
    error = CreateDataBuffer(&receiveBuffer, 7);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("Error: DataBuffer creation failed\n"));
        UninitCommunicationModule();
        return -1;
    }

    CM_SIZE sendSize, receiveSize;
    while (TRUE)
    {
        /*
            Beware that we are using the same CM_SERVER after handling current CM_SERVER_CLIENT.
            CM_SERVER can be reused, it doesn't need to be recreated after each client handling.
            One should find a better handling strategy, since we are only capable to serve one client at a time inside this loop.
        */
        newClient = NULL;

        error = AwaitNewClient(server, &newClient);
        if (CM_IS_ERROR(error)) {
            _tprintf_s(TEXT("Error: A connection failed\n"));
        }

        error = ReceiveDataFromClient(newClient, receiveBuffer, &receiveSize);
        if (CM_IS_ERROR(error) || strcmp((char *)receiveBuffer->DataBuffer, "check") != 0) {
            _tprintf_s(TEXT("Error: A connection failed\n"));
            goto error_connect;
        }
        IncrementConnectedUsers();
        
        EnterCriticalSection(&gConnectedUsers.mutex);
        if (gConnectedUsers.nr > maxNumberOfUsers) {
            CopyDataIntoBuffer(sendBuffer, (const CM_BYTE *)"reject", sizeof("reject"));
            error = SendDataToClient(newClient, sendBuffer, &sendSize);
            if (CM_IS_ERROR(error)) {
                _tprintf_s(TEXT("Error: send reject message failed\n"));
            }
            AbandonClient(newClient);
            gConnectedUsers.nr--;
            LeaveCriticalSection(&gConnectedUsers.mutex);
            continue;
        }
        LeaveCriticalSection(&gConnectedUsers.mutex);

        error = CopyDataIntoBuffer(sendBuffer, (const CM_BYTE *)"accept", sizeof("accept"));
        if (CM_IS_ERROR(error)) {
            _tprintf_s(TEXT("Error: CopyDataIntoBuffer error connection abandoned\n"));
            goto error_connect;
        }

        error = SendDataToClient(newClient, sendBuffer, &sendSize);
        if (CM_IS_ERROR(error)) {
            _tprintf_s(TEXT("Error: send accept message failed connection has been abandoned\n"));
            goto error_connect;
        }


        char *arg[2];
        arg[0]= (char *)newClient;
        arg[1] = (char *)threadPool;
        _tprintf_s(TEXT("CONNECTION START\n"));
        ThreadPoolAddWork(threadPool, StartConnection, arg);
        newClient = NULL;
        continue;
    error_connect:
        AbandonClient(newClient);
        DecrementConnectedUsers();
    }

    _tprintf_s(TEXT("Server is shutting down now...\n"));
    DestroyThreadPool(&threadPool);
    DestroyServer(server);
    UninitCommunicationModule();
    UninitThreadFunctions();
    UninitUsersVector();

    return 0;
}

