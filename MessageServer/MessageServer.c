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

    while (TRUE)
    {
        /*
            Beware that we are using the same CM_SERVER after handling current CM_SERVER_CLIENT.
            CM_SERVER can be reused, it doesn't need to be recreated after each client handling.
            One should find a better handling strategy, since we are only capable to serve one client at a time inside this loop.
        */

        CM_SERVER_CLIENT* newClient = NULL;
        error = AwaitNewClient(server, &newClient);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("AwaitNewClient failed with err-code=0x%X!\n"), error);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        _tprintf_s(TEXT("New client has connected...\n"));

        const char* messageToSend = "-This message is from SERVER-";
        const SIZE_T messageToSendSize = strlen(messageToSend) * sizeof(char);

        CM_DATA_BUFFER* dataToReceive = NULL;
        CM_SIZE dataToReceiveSize = sizeof("-This message is from CLIENT-") * sizeof(char);

        CM_DATA_BUFFER* dataToSend = NULL;
        CM_SIZE dataToSendSize = (CM_SIZE)messageToSendSize;

        error = CreateDataBuffer(&dataToReceive, dataToReceiveSize);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("Failed to create RECEIVE data buffer with err-code=0x%X!\n"), error);
            AbandonClient(newClient);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        error = CreateDataBuffer(&dataToSend, dataToSendSize);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("Failed to create SEND data buffer with err-code=0x%X!\n"), error);
            DestroyDataBuffer(dataToReceive);
            AbandonClient(newClient);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)messageToSend, (CM_SIZE)messageToSendSize);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("CopyDataIntoBuffer failed with err-code=0x%X!\n"), error);
            DestroyDataBuffer(dataToSend);
            DestroyDataBuffer(dataToReceive);
            AbandonClient(newClient);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        CM_SIZE receivedByteCount = 0;
        error = ReceiveDataFromClient(newClient, dataToReceive, &receivedByteCount);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("ReceiveDataFromClient failed with err-code=0x%X!\n"), error);
            DestroyDataBuffer(dataToSend);
            DestroyDataBuffer(dataToReceive);
            AbandonClient(newClient);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        _tprintf_s(TEXT("Successfully received data from client client:\n \tReceived data: \" %.*S \" \n \tReceived data size: %d\n")
            , (int)receivedByteCount
            , (char*)dataToReceive->DataBuffer
            , receivedByteCount
        );

        CM_SIZE sendByteCount = 0;
        error = SendDataToClient(newClient, dataToSend, &sendByteCount);
        if (CM_IS_ERROR(error))
        {
            _tprintf_s(TEXT("SendDataToClient failed with err-code=0x%X!\n"), error);
            DestroyDataBuffer(dataToSend);
            DestroyDataBuffer(dataToReceive);
            AbandonClient(newClient);
            DestroyServer(server);
            UninitCommunicationModule();
            return -1;
        }

        _tprintf_s(TEXT("Successfully send data from client client:\n \tSend data size: %d\n")
            , sendByteCount
        );

        DestroyDataBuffer(dataToSend);
        DestroyDataBuffer(dataToReceive);
        AbandonClient(newClient);
    }

    _tprintf_s(TEXT("Server is shutting down now...\n"));

    DestroyServer(server);
    UninitCommunicationModule();

    return 0;
}

