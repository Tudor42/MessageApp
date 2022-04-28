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

    const char* messageToSend = "-This message is from CLIENT-";
    const SIZE_T messageToSendSize = strlen(messageToSend) * sizeof(char);

    CM_DATA_BUFFER* dataToReceive = NULL;
    CM_SIZE dataToReceiveSize = sizeof("-This message is from SERVER-") * sizeof(char);

    CM_DATA_BUFFER* dataToSend = NULL;
    CM_SIZE dataToSendSize = (CM_SIZE)messageToSendSize;

    error = CreateDataBuffer(&dataToReceive, dataToReceiveSize);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("Failed to create RECEIVE data buffer with err-code=0x%X!\n"), error);
        DestroyClient(client);
        UninitCommunicationModule();
        return -1;
    }

    error = CreateDataBuffer(&dataToSend, dataToSendSize);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("Failed to create SEND data buffer with err-code=0x%X!\n"), error);
        DestroyDataBuffer(dataToReceive);
        DestroyClient(client);;
        UninitCommunicationModule();
        return -1;
    }

    error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)messageToSend, (CM_SIZE)messageToSendSize);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("CopyDataIntoBuffer failed with err-code=0x%X!\n"), error);
        DestroyDataBuffer(dataToSend);
        DestroyDataBuffer(dataToReceive);
        DestroyClient(client);
        UninitCommunicationModule();
        return -1;
    }

    CM_SIZE sendBytesCount = 0;
    error = SendDataToServer(client, dataToSend, &sendBytesCount);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("SendDataToServer failed with err-code=0x%X!\n"), error);
        DestroyDataBuffer(dataToSend);
        DestroyDataBuffer(dataToReceive);
        DestroyClient(client);
        UninitCommunicationModule();
        return -1;
    }

    _tprintf_s(TEXT("Successfully send data to server:\n \t Send data size: %d\n")
        , sendBytesCount
    );

    CM_SIZE receivedByteCount = 0;
    error = ReceiveDataFormServer(client, dataToReceive, &receivedByteCount);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("ReceiveDataFormServer failed with err-code=0x%X!\n"), error);
        DestroyDataBuffer(dataToSend);
        DestroyDataBuffer(dataToReceive);
        DestroyClient(client);
        UninitCommunicationModule();
        return -1;
    }

    _tprintf_s(TEXT("Successfully received data from server:\n \tReceived data: \" %.*S \" \n \tReceived data size: %d\n")
        , (int)receivedByteCount
        , (char*)dataToReceive->DataBuffer
        , receivedByteCount
    );

    _tprintf_s(TEXT("Client is shutting down now...\n"));

    DestroyDataBuffer(dataToSend);
    DestroyDataBuffer(dataToReceive);
    DestroyClient(client);
    UninitCommunicationModule();

    return 0;
}

