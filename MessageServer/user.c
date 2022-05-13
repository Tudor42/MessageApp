#include "stdafx.h"
#include "user.h"

int InitUsersVector(DWORD InitCapacity) {
    gUsersVector.capacity = InitCapacity;
    gUsersVector.size = 0;
    gUsersVector.users = (PUSER)malloc(InitCapacity * sizeof(USER));
    if (gUsersVector.users == NULL) {
        return -1;
    }
    InitializeSRWLock(&gUsersVector.mutex);
    return 0;
}

void UninitUsersVector() {
    free(gUsersVector.users);
}

int UserLoggedName(const char *user) {
    for (DWORD i = 0; i < gUsersVector.size; ++i) {
        if (gUsersVector.users[i].userName != NULL && strcmp(user, gUsersVector.users[i].userName) == 0) return 1;
    }
    return 0;
}

int UserLoggedSocket(CM_SERVER_CLIENT *user) {
    for (DWORD i = 0; i < gUsersVector.size; ++i) {
        if (user == gUsersVector.users[i].client) return 1;
    }
    return 0;
}

int AddUser(CM_SERVER_CLIENT *User, const char *UserName, THREAD_POOL *ThreadPool)
{
    int ret = 0;
    if (UserLoggedSocket(User)) {
        ret = -1;
        goto fin;
    }
    if (UserLoggedName(UserName)) {
        ret = -2;
        goto fin;
    }

    gUsersVector.users[gUsersVector.size].client = User;
    gUsersVector.users[gUsersVector.size].userName = (char *)malloc(sizeof(UserName) + 1);
    if (gUsersVector.users[gUsersVector.size].userName == NULL) {
        ret = -3;
        goto fin;
    }
    strcpy(gUsersVector.users[gUsersVector.size].userName, UserName);
    gUsersVector.users[gUsersVector.size].threadPool = ThreadPool;
    gUsersVector.size++;
fin:
    return ret;
}

int RemoveUser(CM_SERVER_CLIENT *User) {
    for (DWORD i = 0; i < gUsersVector.size; ++i) {
        if (User == gUsersVector.users[i].client) {
            free(gUsersVector.users[i].userName);
            if (i + 1 != gUsersVector.size) {
                for (DWORD j = i; j < gUsersVector.size - 1; ++j) {
                    gUsersVector.users[j] = gUsersVector.users[j + 1];
                }
            }
            --gUsersVector.size;
            return 0;
        }
    }
    return -1;
}