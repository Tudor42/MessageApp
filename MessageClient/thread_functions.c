#include "stdafx.h"
#include "thread_functions.h"


void InitThreadFunctions() {
    InitReadWriteBuff();
    gControlVar.stop = false;
    InitializeCriticalSection(&gControlVar.mutex);
}

void UninitThreadFunctions() {
    UninitReadWriteBuff();
    DeleteCriticalSection(&gControlVar.mutex);
}

void InitReadWriteBuff() {
    gReadBuff.capacity = 64;
    gReadBuff.size = 1;
    gReadBuff.buff = malloc(gReadBuff.capacity * sizeof(char));
    gReadBuff.buff[0] = '$';
    InitializeCriticalSection(&gReadBuff.mutex);
}

void UninitReadWriteBuff() {
    free(gReadBuff.buff);
    DeleteCriticalSection(&gReadBuff.mutex);
}

void ReadConsoleThread() {
    TCHAR d;
    DWORD size;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), TEXT("$"), sizeof(TCHAR), &size, NULL);
    do {
        ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &d, 1, &size, NULL);
        EnterCriticalSection(&gReadBuff.mutex);
        if (gReadBuff.size >= gReadBuff.capacity) {
            gReadBuff.capacity *= 2;
            char *tmp = realloc(gReadBuff.buff, gReadBuff.capacity * sizeof(char));
            if (tmp != NULL) {
                gReadBuff.buff = tmp;
            }
        }
        if(d != '\b'){
            if (gReadBuff.size < MAX_BUFF_SIZE) {
                gReadBuff.buff[gReadBuff.size++] = (char)d;
                WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &d, 1, &size, NULL);
            }
        }
        else {
            if (gReadBuff.size > 1) {
                TCHAR tmp[] = TEXT("\b \b");
                WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &tmp, 3, &size, NULL);
                gReadBuff.size--;
            }
        }
        if (d == '\r')
            WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), TEXT("\n"), sizeof(TCHAR), &size, NULL);
        LeaveCriticalSection(&gReadBuff.mutex);
    } while (size != 0 && d != '\r' && !gControlVar.stop);
}

void WriteBeforeReadBuffer(const char *Data, DWORD DataSize) {
    DWORD size;
    TCHAR d = ' ';
    EnterCriticalSection(&gReadBuff.mutex);

    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), TEXT("\r"), 1, &size, NULL);
    for (DWORD i = 0; i < gReadBuff.size; ++i) {
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &d, 1, &size, NULL);
    }
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), TEXT("\r"), 1, &size, NULL);
    for (DWORD i = 0; i < DataSize; ++i) {
        d = (TCHAR)Data[i];
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &d, 1, &size, NULL);
    }
    if (gReadBuff.size != 0) {
        for (DWORD i = 0; i < gReadBuff.size; ++i) {
            d = (TCHAR)gReadBuff.buff[i];
            WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), &d, 1, &size, NULL);
        }
    }
    LeaveCriticalSection(&gReadBuff.mutex);
}

void SendReadBuffer(CM_CLIENT *client) {
    EnterCriticalSection(&gReadBuff.mutex);
    CM_DATA_BUFFER *data;
    CM_ERROR error = CreateDataBuffer(&data, gReadBuff.size + 1);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("Error: DataBuffer creation fail\n"));
        LeaveCriticalSection(&gReadBuff.mutex);
        return;
    }
    gReadBuff.buff[gReadBuff.size] = 0;
    CopyDataIntoBuffer(data, (const CM_BYTE *)gReadBuff.buff + 1, gReadBuff.size);
    CM_SIZE size;
    error = SendDataToServer(client, data, &size);
    if (CM_IS_ERROR(error)) {
        _tprintf_s(TEXT("Error: ReadBuffer send to server fail\n"));
        LeaveCriticalSection(&gReadBuff.mutex);
        return;
    }

    gReadBuff.size = 1;
    LeaveCriticalSection(&gReadBuff.mutex);
}

void StartConsole(void *args) {
    CM_CLIENT *client = args;
    while (!gControlVar.stop) {
        ReadConsoleThread();
        SendReadBuffer(client);
    }
}

void Parse(char *str, CM_SIZE size, char **words, int *nrWords) {
    //
    // 
    //

    *nrWords = 0;
    int blank = 1;
    CM_SIZE j = 0;
    //printf("%s %d\n", str, (int)strlen(str));

    while (*str && j < size) {
        if (blank && !isblank(*str)) {
            words[*nrWords] = str;
            *nrWords += 1;
            blank = 0;
        }
        else if (!blank && isblank(*str)) {
            blank = 1;
        }
        ++str;
        ++j;
    }

}

void GetDataServer(void *args) {
    CM_CLIENT *client = args;
    THREAD_POOL *threadPool = NULL;
    (void)threadPool;
    CM_DATA_BUFFER *receiveBuffer;
    CM_ERROR error = CreateDataBuffer(&receiveBuffer, MAX_BUFF_SIZE);
    if (CM_IS_ERROR(error)) {
        _tprintf(TEXT("Error: receiveBuffer creation failed\n"));
        EnterCriticalSection(&gControlVar.mutex);
        gControlVar.stop = true;
        LeaveCriticalSection(&gControlVar.mutex);
        return;
    }
    char *words[MAX_BUFF_SIZE / 2 + 1];
    int nrWords;
    while (!gControlVar.stop) {
        CM_SIZE size;
        error = ReceiveDataFormServer(client, receiveBuffer, &size);
        if (CM_IS_ERROR(error)) {
            _tprintf(TEXT("Error: unexpected server error\n"));
            EnterCriticalSection(&gControlVar.mutex);
            gControlVar.stop = true;
            LeaveCriticalSection(&gControlVar.mutex);
            return;
        }
        receiveBuffer->DataBuffer[size] = 0;
        Parse((char *)receiveBuffer->DataBuffer, size, words, &nrWords);
        if (nrWords != 0) {
            // print data
            // getfile [filename] [packetnum] data
            // close connection
            char *tmp = words[0];
            while (*tmp && !isblank(*tmp)) {
                ++tmp;
            }
            *tmp = 0;
            if (strcmp(words[0], "print") == 0) {
                if(nrWords > 1)
                    WriteBeforeReadBuffer(words[0] + strlen(words[0]) + 1, (DWORD)strlen(words[1]) + 1);
                continue;
            }
            if (strcmp(words[0], "close") == 0) {
                tmp = words[1];
                while (*tmp && !isblank(*tmp)) {
                    ++tmp;
                }
                *tmp = 0;
                if (strcmp(words[1], "connection") == 0) {
                    EnterCriticalSection(&gControlVar.mutex);
                    gControlVar.stop = true;
                    LeaveCriticalSection(&gControlVar.mutex);
                    _tprintf(TEXT("\nServer closed connection\n"));
                    exit(0);
                    return;
                }
                continue;
            }

        }
    }
}