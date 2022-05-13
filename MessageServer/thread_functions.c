#include "stdafx.h"
#include "thread_functions.h"
#include <stdio.h>
#include <tchar.h>
#include "thread_pool.h"
#include "user.h"

int InitThreadFunctions() {

    gConnectedUsers.nr = 0;
    InitializeCriticalSection(&gConnectedUsers.mutex);
    InitializeSRWLock(&gRegistrationFileMutex);
    return 0;
}

void UninitThreadFunctions() {
    DeleteCriticalSection(&gConnectedUsers.mutex);
    
    CloseHandle(gRegistrationFile);
}

int RegisterNewUser(char *UserName, char *Password, CM_SERVER_CLIENT *client) {
    AcquireSRWLockShared(&gUsersVector.mutex);
    if (UserLoggedSocket(client)) {
        ReleaseSRWLockShared(&gUsersVector.mutex);
        return -5;
    }
    ReleaseSRWLockShared(&gUsersVector.mutex);
    AcquireSRWLockExclusive(&gRegistrationFileMutex);
    char *tmp = UserName;
    while (!isblank(*tmp)) {
        ++tmp;
    }
    *tmp = 0;
    tmp = UserName;
    while (*tmp) {
        if (!isalnum(*tmp)) {
            ReleaseSRWLockExclusive(&gRegistrationFileMutex);
            return -1;
        }
        ++tmp;
    }

    tmp = Password;
    while (*tmp && *tmp != '\r' && !isblank(*tmp)) {
        ++tmp;
    }
    *tmp = 0;

    int bigLetter = 0, nonAlnum = 0, size = 0;
    tmp = Password;
    while (*tmp) {
        ++size;
        if (!isalnum(*tmp)) nonAlnum = 1;
        if (*tmp == ' ' || *tmp == ',') { 
            ReleaseSRWLockExclusive(&gRegistrationFileMutex);
            return -2;
        }
        if ('A' <= *tmp && *tmp <= 'Z') bigLetter = 1;
        ++tmp;
    }
    if (size < 5 || bigLetter == 0 || nonAlnum == 0) {
        ReleaseSRWLockExclusive(&gRegistrationFileMutex);
        return -4;
    }
    gRegistrationFile = CreateFile(
        TEXT("registration.txt"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == gRegistrationFile) {
        ReleaseSRWLockExclusive(&gRegistrationFileMutex);
        return -6;
    }

    LARGE_INTEGER offset, np;
    offset.HighPart = 0;
    offset.LowPart = 0;
    SetFilePointerEx(gRegistrationFile, offset, &np, FILE_BEGIN);
    char ch;
    DWORD read;
    DWORD buffSize = 0;
    char charBuffer[MAX_BUFF_SIZE];
    while (ReadFile(gRegistrationFile, &ch, 1, &read, NULL) && read != 0) {
        if (ch != '\r' && ch != '\n') {
            charBuffer[buffSize++] = ch;
            continue;
        }
        if (ch == '\n') {
            charBuffer[buffSize] = 0;
            for (DWORD i = 0; i < buffSize; ++i) {
                if (charBuffer[i] == ' ') {
                    charBuffer[i] = 0;
                    break;
                }
            }
            if (strcmp(charBuffer, UserName) == 0) {
                CloseHandle(gRegistrationFile);
                ReleaseSRWLockExclusive(&gRegistrationFileMutex);
                return -3;
            }
            buffSize = 0;
        }
    }

    tmp = UserName;
    buffSize = 0;
    while (*tmp) {
        
        charBuffer[buffSize] = *tmp;
        buffSize++;
        ++tmp;
    }

    charBuffer[buffSize++] = ' ';
    tmp = Password;
    while (*tmp) {
        
        charBuffer[buffSize] = *tmp;
        buffSize++;
        ++tmp;
    }
    charBuffer[buffSize++] = '\r';
    charBuffer[buffSize++] = '\n';
    buffSize++;
    charBuffer[buffSize] = 0;
    SetFilePointerEx(gRegistrationFile, offset, &np, FILE_END);
    WriteFile(gRegistrationFile, charBuffer, buffSize - 1, &read, NULL);
    SetEndOfFile(gRegistrationFile);
    CloseHandle(gRegistrationFile);
    ReleaseSRWLockExclusive(&gRegistrationFileMutex);
    return 0;
}

int LoginUser(char *UserName, char *Password, CM_SERVER_CLIENT *Client, THREAD_POOL *ThreadPool) {
    AcquireSRWLockExclusive(&gUsersVector.mutex);
    if(UserLoggedSocket(Client)){
        ReleaseSRWLockExclusive(&gUsersVector.mutex);
        return -2;
    }

    if (UserLoggedName(UserName)) {
        ReleaseSRWLockExclusive(&gUsersVector.mutex);
        return -1;
    }

    AcquireSRWLockExclusive(&gRegistrationFileMutex);
    gRegistrationFile = CreateFile(
        TEXT("registration.txt"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == gRegistrationFile) {
        ReleaseSRWLockExclusive(&gUsersVector.mutex);
        ReleaseSRWLockExclusive(&gRegistrationFileMutex);
        return -6;
    }

    LARGE_INTEGER offset, np;
    offset.HighPart = 0;
    offset.LowPart = 0;
    SetFilePointerEx(gRegistrationFile, offset, &np, FILE_BEGIN);
    char ch;
    DWORD read;
    DWORD buffSize = 0;
    char charBuffer[MAX_BUFF_SIZE];
    char *tmp2 = UserName;
    while (*tmp2 && !isblank(*tmp2)) {
        ++tmp2;
    }
    *tmp2 = 0;
    tmp2 = Password;
    while (*tmp2 && *tmp2 != '\r' && !isblank(*tmp2)) {
        ++tmp2;
    }
            *tmp2 = 0;
    while (ReadFile(gRegistrationFile, &ch, 1, &read, NULL) && read != 0) {
        if (ch != '\r' && ch != '\n') {
            charBuffer[buffSize++] = ch;
            continue;
        }
        if (ch == '\n') {
            charBuffer[buffSize] = 0;
            char *tmp = charBuffer;
            for (DWORD i = 0; i < buffSize; ++i) {
                if (charBuffer[i] == ' ') {
                    tmp = (charBuffer + i + 1);
                    charBuffer[i] = 0;
                    break;
                }
            }
            if (strcmp(charBuffer, UserName) == 0) {
                if (strcmp(tmp, Password) == 0) {
                    if (AddUser(Client, UserName, ThreadPool) == -2) {
                        ReleaseSRWLockExclusive(&gUsersVector.mutex);
                        CloseHandle(gRegistrationFile);
                        ReleaseSRWLockExclusive(&gRegistrationFileMutex);
                        return -2;
                    }
                    else {
                        ReleaseSRWLockExclusive(&gUsersVector.mutex);
                        CloseHandle(gRegistrationFile);
                        ReleaseSRWLockExclusive(&gRegistrationFileMutex);
                        return 0;
                    }
                }
                else {
                    ReleaseSRWLockExclusive(&gUsersVector.mutex);
                    CloseHandle(gRegistrationFile);
                    ReleaseSRWLockExclusive(&gRegistrationFileMutex);
                    return -3;
                }
            }
            buffSize = 0;
        }
    }
    CloseHandle(gRegistrationFile);
    ReleaseSRWLockExclusive(&gRegistrationFileMutex);
    ReleaseSRWLockExclusive(&gUsersVector.mutex);
    return -3;
}

CM_SERVER_CLIENT *GetLogedUserSocket(char *UserName) {
    AcquireSRWLockShared(&gUsersVector.mutex);
    for (DWORD i = 0; i < gUsersVector.size; ++i) {
        if (strcmp(UserName, gUsersVector.users[i].userName) == 0) {
            ReleaseSRWLockShared(&gUsersVector.mutex);
            return gUsersVector.users[i].client;
        }
    }

    ReleaseSRWLockShared(&gUsersVector.mutex);
    return NULL;
}

void IncrementConnectedUsers(){
    EnterCriticalSection(&gConnectedUsers.mutex);
    gConnectedUsers.nr += 1;
    LeaveCriticalSection(&gConnectedUsers.mutex);
}

void DecrementConnectedUsers(){
    EnterCriticalSection(&gConnectedUsers.mutex);
    gConnectedUsers.nr -= 1;
    LeaveCriticalSection(&gConnectedUsers.mutex);
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

void GetCmd(void *arg) {
    CM_SIZE size;
    CM_SERVER_CLIENT *client = ((void **)arg)[0];
    THREAD_POOL *threadPool = ((void **)arg)[1];
    CM_DATA_BUFFER *recv;
    CreateDataBuffer(&recv, MAX_BUFF_SIZE);
    CM_ERROR error;

    char buffer[MAX_BUFF_SIZE];
    char *words[MAX_BUFF_SIZE / 2 + 1];
    int nrWords;
    do {
        error = ReceiveDataFromClient(client, recv, &size);
        if (CM_IS_ERROR(error)) {
            _tprintf(TEXT("Error client socket\n"));
            AcquireSRWLockExclusive(&gUsersVector.mutex);
            RemoveUser(client);
            ReleaseSRWLockExclusive(&gUsersVector.mutex);
            AbandonClient(client);
            DecrementConnectedUsers();
            return;
        }
        Parse((char *)recv->DataBuffer, size, words, &nrWords);

        if (nrWords != 0) {
            // print data
            // getfile [filename] [packetnum] data
            // close connection
            char *tmp = words[0];
            while (*tmp && !isblank(*tmp) && *tmp != '\r') {
                ++tmp;
            }
            *tmp = 0;

            if (strcmp(words[0], "echo") == 0) {
                if (nrWords > 1) {
                    _tprintf_s(TEXT("%S\n"), tmp + 1);
                    strcpy(buffer, "print ");
                    strcpy(buffer + 6, tmp + 1);
                    strcpy(buffer + 6 + strlen(tmp + 1) - 1, "\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                }
                continue;
            }

            if (strcmp(words[0], "list") == 0) {
                if (nrWords == 1) {
                    AcquireSRWLockShared(&gUsersVector.mutex);
                    for (DWORD i = 0; i < gUsersVector.size; ++i) {
                        strcpy(buffer, "print ");
                        strcpy(buffer + strlen(buffer), gUsersVector.users[i].userName);
                        strcpy(buffer + strlen(buffer), "\n");
                        _tprintf_s(L"%S", buffer);
                        CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                        SendDataToClient(client, recv, &size);
                    }
                    ReleaseSRWLockShared(&gUsersVector.mutex);
                }
                else {
                    strcpy(buffer, "print Error: Command list takes no arguments\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                }
                continue;
            }

            if (strcmp(words[0], "broadcast") == 0) {
                if (nrWords > 1) {
                    AcquireSRWLockShared(&gUsersVector.mutex);
                    if (!UserLoggedSocket(client)) {
                        strcpy(buffer, "print Error: No user currently logged in\n");
                        CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                        ReleaseSRWLockShared(&gUsersVector.mutex);
                        SendDataToClient(client, recv, &size);
                        continue;
                    }
                    DWORD i = 0;
                    for (i = 0; i < gUsersVector.size; ++i) {
                        if (client == gUsersVector.users[i].client) {
                            break;
                        }
                    }

                    strcpy(buffer, "print Broadcast from ");
                    strcpy(buffer + strlen(buffer), gUsersVector.users[i].userName);
                    strcpy(buffer + strlen(buffer), ": ");
                    strcpy(buffer + strlen(buffer), tmp + 1);
                    strcpy(buffer + strlen(buffer) - 1, "\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    for (i = 0; i < gUsersVector.size; ++i) {
                        if(client != gUsersVector.users[i].client)
                            SendDataToClient(gUsersVector.users[i].client, recv, &size);
                    }
                    strcpy(buffer, "print Succes\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                    ReleaseSRWLockShared(&gUsersVector.mutex);
                }
                else {
                    strcpy(buffer, "print Error: broadcast missing message\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                }
                continue;
            }

            if (strcmp(words[0], "msg") == 0) {
                if (nrWords >= 3) {
                    AcquireSRWLockShared(&gUsersVector.mutex);
                    if (!UserLoggedSocket(client)) {
                        strcpy(buffer, "print Error: No user currently logged in\n");
                        CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                        ReleaseSRWLockShared(&gUsersVector.mutex);
                        SendDataToClient(client, recv, &size);
                        continue;
                    }
                    char *tmp2 = words[1];
                    while (*tmp2 && *tmp2 != ' ') {
                        ++tmp2;
                    }
                    *tmp2 = 0;
                    CM_SERVER_CLIENT *receiver = GetLogedUserSocket(words[1]);
                    if (receiver == NULL) {
                        strcpy(buffer, "print Error: User not logged in\n");
                        CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                        ReleaseSRWLockShared(&gUsersVector.mutex);
                        SendDataToClient(client, recv, &size);
                        continue;
                    }
                    DWORD i = 0;
                    for (i = 0; i < gUsersVector.size; ++i) {
                        if (client == gUsersVector.users[i].client) {
                            break;
                        }
                    }

                    strcpy(buffer, "print Message from ");
                    strcpy(buffer + strlen(buffer), gUsersVector.users[i].userName);
                    strcpy(buffer + strlen(buffer), ": ");
                    strcpy(buffer + strlen(buffer), tmp2 + 1);
                    strcpy(buffer + strlen(buffer) - 1, "\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(receiver, recv, &size);
                    ReleaseSRWLockShared(&gUsersVector.mutex);
                    strcpy(buffer, "print Succes\n");
                }
                else {
                    if (nrWords < 2) {
                        strcpy(buffer, "print User and message missing\n");
                    }
                    else {
                        strcpy(buffer, "print Message missing\n");
                    }
                }
                CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                SendDataToClient(client, recv, &size);
                continue;
            }


            if (strcmp(words[0], "register") == 0) {
                if (nrWords > 3) {
                    strcpy(buffer, "print Command used improperly\nUsername and password cannot contain spaces\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                    continue;
                }
                if (nrWords < 3) {
                    strcpy(buffer, "print Command used improperly\nUsername and password missing\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                    continue;
                }
                int ret = RegisterNewUser(words[1], words[2], client);
                switch(ret){
                case 0:
                    strcpy(buffer, "print Succes\n");
                    break;
                case -1:
                    strcpy(buffer, "print Error: Invalid username\n");
                    break;
                case -2:
                    strcpy(buffer, "print Error: Invalid password\n");
                    break;
                case -3:
                    strcpy(buffer, "print Error: Username already registered\n");
                    break;
                case -4:
                    strcpy(buffer, "print Error: Password too weak\n");
                    break;
                case -5:
                    strcpy(buffer, "print Error: User already logged in\n");
                    break;
                default:
                    strcpy(buffer, "print Error: Server error\n");
                    break;
                }
                CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                SendDataToClient(client, recv, &size);
                continue;
            }

            if (strcmp(words[0], "login") == 0) {
                if (nrWords > 3) {
                    strcpy(buffer, "print Command used improperly\nUsername and password cannot contain spaces\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                    continue;
                }
                if (nrWords < 3) {
                    strcpy(buffer, "print Command used improperly\nUsername and password missing\n");
                    CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                    SendDataToClient(client, recv, &size);
                    continue;
                }
                int ret = LoginUser(words[1], words[2], client, threadPool);
                switch (ret) {
                case 0:
                    strcpy(buffer, "print Succes\n");
                    break;
                case -1:
                    strcpy(buffer, "print Error: User already logged in\n");
                    break;
                case -2:
                    strcpy(buffer, "print Error: Another user already logged in\n");
                    break;
                case -3:
                    strcpy(buffer, "print Error: Invalid username/password combination\n");
                    break;
                default:
                    strcpy(buffer, "print Error: Server error\n");
                    break;
                }
                CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                SendDataToClient(client, recv, &size);
                continue;
            }

            if (strcmp(words[0], "exit") == 0) {
                strcpy(buffer, "close connection");
                CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                SendDataToClient(client, recv, &size);
                AcquireSRWLockExclusive(&gUsersVector.mutex);
                RemoveUser(client);
                ReleaseSRWLockExclusive(&gUsersVector.mutex);
                AbandonClient(client);
                DecrementConnectedUsers();
                return;
            }

            if (strcmp(words[0], "logout") == 0) {
                AcquireSRWLockExclusive(&gUsersVector.mutex);
                int ret = RemoveUser(client);
                ReleaseSRWLockExclusive(&gUsersVector.mutex);
                switch (ret) {
                case 0:
                    strcpy(buffer, "print Succes\n");
                    break;
                case -1:
                    strcpy(buffer, "print Error: No user currently logged in\n");
                    break;
                default:
                    strcpy(buffer, "print Error: Server error\n");
                    break;
                }
                CopyDataIntoBuffer(recv, (const CM_BYTE *)buffer, (CM_SIZE)strlen(buffer));
                SendDataToClient(client, recv, &size);
                continue;
            }
            error = SendDataToClient(client, recv, &size);
            if (CM_IS_ERROR(error)) {
                _tprintf(TEXT("Error client socket\n"));
                AcquireSRWLockExclusive(&gUsersVector.mutex);
                RemoveUser(client);
                ReleaseSRWLockExclusive(&gUsersVector.mutex);
                AbandonClient(client);
                DecrementConnectedUsers();

                return;
            }
        }
    } while (size != 0);
    AcquireSRWLockExclusive(&gUsersVector.mutex);
    RemoveUser(client);
    ReleaseSRWLockExclusive(&gUsersVector.mutex);
    AbandonClient(client);
    DecrementConnectedUsers();
}

void StartConnection(void *arg) {
    if (arg == NULL) {
        return;
    }

    CM_SERVER_CLIENT *client = ((void **)arg)[0];
    THREAD_POOL *threadPool = ((void **)arg)[1];
    
    if (ThreadPoolAddWork(threadPool, GetCmd, arg) == -1) {
        free(arg);
        AbandonClient(client);
        DecrementConnectedUsers();
        _tprintf_s(TEXT("CLOSED CONNECTION\n"));
    }
}

