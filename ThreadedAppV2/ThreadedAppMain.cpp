/**
*   ThreadedAppMain.cpp
*   Copyright (c) 2016 Igor Egorov
*   Total Virtualization course @ Innopolis University
*   https://github.com/dxyozh/vsmp_perf
*/

#include "ThreadsManager.h"
#include "PracticalSocket.h"
#include <iostream>
#include <Windows.h>

ThreadsManager man;
HANDLE hThreadsReady(NULL);
HANDLE hThreadsEnded(NULL);

HANDLE gMutex(NULL);
long long shared;

#define PORT 53288
#define ITERS 100000

const char *commands[] = {
    "EXP_1", //0
    "EXP_2", //1
    "READY", //2
    "START", //3
    "EDONE", //4
    "ERROR", //5
};

#define CMDLEN(N) commands[(N)], strlen(commands[(N)])

void ThreadsReady() {
    SetEvent(hThreadsReady);
}

void ThreadsEnd() {
    SetEvent(hThreadsEnded);
}

void Routine() {
    int threadIterations(ITERS);
    DWORD wr;
    bool ret = false;

    while (threadIterations) {
        wr = WaitForSingleObject(gMutex, INFINITE);
        switch (wr)
        {
        case WAIT_OBJECT_0:
            __try {
                --threadIterations;
                ++shared;
            }
            __finally {
                if (!ReleaseMutex(gMutex))
                {
                    printf_s("Error: unable to release mutex\n");
                    ret = true;
                }
            }
            if (ret) {
                return;
            }
            break;
        case WAIT_ABANDONED:
            printf_s("Error: mutex abandoned\n");
            return;
        }
    }
}

const int gBufSize = 128;
char gBuf[gBufSize];

void recv_ans(TCPSocket *psock) {
    int totalBytesReceived(0);
    int bytesReceived(0);

    const int cmdLen(5);

    while (totalBytesReceived < cmdLen) {
        bytesReceived = psock->recv(gBuf + totalBytesReceived, gBufSize - totalBytesReceived);
        totalBytesReceived += bytesReceived;
    }
}

int HandleRequest(TCPSocket *sock) {

    memset(gBuf, 0, gBufSize);
    shared = 0;

    recv_ans(sock);
    if (!strncmp(gBuf, CMDLEN(0))) {
        man.CreateThreads(2, true); //distinct_cpu will be false if machine would not have required amount of CPUs
    } else if (!strncmp(gBuf, CMDLEN(1))) {
        man.CreateThreads(2, false);
    } else {
        printf_s("Error occured. Experiment cancelled\n");
        return 1;
    }

    if (WAIT_OBJECT_0 != WaitForSingleObject(hThreadsReady, INFINITE)) {
        printf_s("Error occured. Experiment cancelled\n");
        return 1;
    }
    ResetEvent(hThreadsReady);

    sock->send(CMDLEN(2));

    recv_ans(sock);

    if (strncmp(gBuf, CMDLEN(3))) {
        printf_s("Error occured. Experiment cancelled\n");
        return 1;
    }

    man.StartThreads();

    if (WAIT_OBJECT_0 != WaitForSingleObject(hThreadsEnded, INFINITE)) {
        printf_s("Error occured. Experiment cancelled\n");
        return 1;
    }
    ResetEvent(hThreadsEnded);

    if (shared != ITERS * 2) {
        sock->send(CMDLEN(5));
        printf_s("Multithreaded computation was wrong\n");
        return 1;
    } else {
        sock->send(CMDLEN(4));
    }

    delete sock;
    return 0;
}

int main(int argc, const char *argv[]) {
    man.SetRoutine(Routine);
    man.SetThreadsReadyCallback(ThreadsReady);
    man.SetEndCallback(ThreadsEnd);

    if (!(hThreadsReady = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        printf_s("Error: cannot create event\n");
        return 1;
    }
    if (!(hThreadsEnded = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        printf_s("Error: cannot create event\n");
        return 1;
    }

    if (!(gMutex = CreateMutex(NULL, FALSE, NULL))) {
        printf_s("Error: cannot create event\n");
        return 1;
    }


    try {
        TCPServerSocket servSock(PORT);
        std::vector<std::string> addrs;
        servSock.getInterfaces(addrs);

        for (std::string &address : addrs) {
            printf_s("%s\n", address.c_str());
        }

        for (;;) {
            if (HandleRequest(servSock.accept())) {
                break;
            }
        }
    } catch (SocketException &e) {
        printf_s("Exception: %s\n", e.what());
        return 1;
    }

    SAFE_CLOSE_HANDLE(hThreadsReady);
    SAFE_CLOSE_HANDLE(hThreadsEnded);
    SAFE_CLOSE_HANDLE(gMutex);
    return 0;
}
