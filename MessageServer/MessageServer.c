// MessageServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
// communication library
#include "communication_api.h"

#include <windows.h>

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

    CM_SIZE maxNumberOfUsers = 0, connectedUsers = 0; 

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

    _tprintf_s(TEXT("Server is Up & Running...\n"));
    char str[] = "dnsua dndsua dnsiadndnd    sjdai", *tmp;
    
    
    tmp = strtok(str, " ");

    while (tmp) {
        printf("%s\n", tmp);
        tmp = strtok(NULL, " ");
    }

    while (TRUE)
    {
        /*
            Beware that we are using the same CM_SERVER after handling current CM_SERVER_CLIENT.
            CM_SERVER can be reused, it doesn't need to be recreated after each client handling.
            One should find a better handling strategy, since we are only capable to serve one client at a time inside this loop.
        */

        CM_SERVER_CLIENT* newClient = NULL;
        if (connectedUsers < maxNumberOfUsers) {
            error = AwaitNewClient(server, &newClient);
            _tprintf_s(TEXT("Client connected\n"));
            if (CM_IS_ERROR(error))
            {
                _tprintf_s(TEXT("AwaitNewClient failed with err-code=0x%X!\n"), error);
                DestroyServer(server);
                UninitCommunicationModule();
                return -1;
            }
            CM_DATA_BUFFER *recvBuff = NULL;
            CM_SIZE buffSize;
            error = CreateDataBuffer(&recvBuff, 12);
            if (CM_IS_ERROR(error)) {
                _tprintf_s(TEXT("Data buffer creation error\n"));
                AbandonClient(newClient);
                continue;
            }

            error = ReceiveDataFromClient(newClient, recvBuff, &buffSize);
            if (CM_IS_ERROR(error) || buffSize == 0) {
                _tprintf_s(TEXT("Check failed %.*S\n"), buffSize, (char *) recvBuff->DataBuffer);
                AbandonClient(newClient);
                continue;
            }
            ++connectedUsers;
        }

    }

    _tprintf_s(TEXT("Server is shutting down now...\n"));

    DestroyServer(server);
    UninitCommunicationModule();

    return 0;
}

