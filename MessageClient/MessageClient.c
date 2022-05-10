// MessageClient.cpp : Defines the entry point for the console application.
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

    EnableCommunicationModuleLogger();

    CM_ERROR error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
        return -1;
    }

    CM_CLIENT* client = NULL;
    error = CreateClientConnectionToServer(&client);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("CreateClientConnectionToServer failed with err-code=0x%X!\n"), error);
        UninitCommunicationModule();
        return -1;
    }

    _tprintf_s(TEXT("We are connected to the server...\n"));

    CM_DATA_BUFFER *sendBuffer = NULL;
    error = CreateDataBuffer(&sendBuffer, (CM_SIZE) 20);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("DataBuffer failed with err-code=0x%X!\n"), error);
        goto finish;
    }
    const char *sec = "check";
    CopyDataIntoBuffer(sendBuffer, (const CM_BYTE *)sec, 6 * sizeof(char));
    
    CM_SIZE sendSize;

    _tprintf_s(TEXT("%.*S"), (int)(6 * sizeof(char)), (char *)sendBuffer->DataBuffer);
    error = SendDataToServer(client, send, &sendSize);


    if (CM_IS_ERROR(error) || sendSize == 0) {
        _tprintf_s(TEXT("Connection rejected\n"));
        goto finish;
    }


    while (1) {
        CM_BYTE *command;
        CM_SIZE n, buffsize;
        n = getline(&command, &buffsize, stdin);

        
    }


finish:
    _tprintf_s(TEXT("Client is shutting down now...\n"));
    DestroyClient(client);
    UninitCommunicationModule();

    return 0;
}

