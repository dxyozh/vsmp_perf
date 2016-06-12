/**
*   ControllerMain.cpp
*   Copyright (c) 2016 Igor Egorov
*   Total Virtualization course @ Innopolis University
*   https://github.com/dxyozh/vsmp_perf
*/

#include "PracticalSocket.h"
#include <iostream>
#include <cstdlib>
#include <chrono>

#define PORT 53288

const char *commands[] = {
    "EXP_1", //0
    "EXP_2", //1
    "READY", //2
    "START", //3
    "EDONE", //4
    "ERROR", //5
};

#define CMDLEN(N) commands[(N)], strlen(commands[(N)])
#define NOW() std::chrono::high_resolution_clock::now()

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

int main(int argc, char *argv[]) {

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <ServerAddr>" << endl;
        return 1;
    }

    memset(gBuf, 0, gBufSize);
    string servAddress = argv[1];
    long long results[2] = {0, 0};

    try {
        for (int i = 0; i < 2; ++i) {
            TCPSocket sock(servAddress, PORT);
            std::chrono::high_resolution_clock::time_point start, end;

            sock.send(CMDLEN(i));
            recv_ans(&sock);

            if (strncmp(gBuf, CMDLEN(2))) {
                printf_s("Wrong answer received. Terminating\n");
                return 1;
            }

            printf_s("Experiment %d\n", i + 1);
            sock.send(CMDLEN(3));

            start = NOW();

            recv_ans(&sock);
            if (!strncmp(gBuf, CMDLEN(4))) {
                end = NOW();
                results[i] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                printf_s("Experiment %d took %d ms\n", i + 1, results[i]);
            } else if (!strncmp(gBuf, CMDLEN(5))) {
                printf_s("Experiment was failed\n");
            } else {
                printf_s("Wrong answer received. Terminating\n");
                return 1;
            }
        }


    } catch(SocketException &e) {
        printf_s("Exception: %s\n", e.what());
        return 1;
    }

    return 0;
}