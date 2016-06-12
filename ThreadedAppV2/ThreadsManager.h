/**
*   ThreadsManager.h
*   Copyright (c) 2016 Igor Egorov
*   Total Virtualization course @ Innopolis University
*   https://github.com/dxyozh/vsmp_perf
*/

#pragma once
#include <vector>
#include <Windows.h>
#include <cstdio>
#include <mutex>
#include <condition_variable>

#define SAFE_CLOSE_HANDLE(H) do { if ((H)) { CloseHandle((H)); }} while (0)

class ThreadsManager;

struct ManagedThread {
    int m_iTargetCpu;
    HANDLE m_hThread;
    HANDLE m_hReadyEvent;
    ThreadsManager *m_pManager;

    ManagedThread(ThreadsManager *pManager, int iTargetCpu);
    ~ManagedThread();
};

class ThreadsManager
{
    void (*RoutineFunc)();
    void (*ThreadsReadyCallback)();
    void (*ThreadEndCallback)();
    std::vector<struct ManagedThread*> pthreads;
    std::mutex lock;
    std::condition_variable cv_start;
    bool bThreadsStarted;

    std::mutex io_mutex;

    static DWORD WINAPI Routine(LPVOID params);

public:
    ThreadsManager();
    ~ThreadsManager();
    void SetRoutine(void (*fRoutine)());
    void SetThreadsReadyCallback(void (*fThreadsReadyCb)());
    void SetEndCallback(void (*fEndCb)());
    void CreateThreads(int threads_count, bool distinct_cpu);
    void StartThreads();
};
