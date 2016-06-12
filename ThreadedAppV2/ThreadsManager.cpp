/**
*   ThreadsManager.cpp
*   Copyright (c) 2016 Igor Egorov
*   Total Virtualization course @ Innopolis University
*   https://github.com/dxyozh/vsmp_perf
*/

#include "ThreadsManager.h"

ThreadsManager::ThreadsManager():
    RoutineFunc(NULL),
    ThreadEndCallback(NULL),
    bThreadsStarted(false)
{
}

ThreadsManager::~ThreadsManager()
{
    for (auto &t: pthreads) {
        if (t) {
            delete t;
        }
    }
}

void ThreadsManager::SetRoutine(void (*fRoutine)()) {
    RoutineFunc = fRoutine;
}

void ThreadsManager::SetThreadsReadyCallback(void (*fThreadsReadyCb)()) {
    ThreadsReadyCallback = fThreadsReadyCb;
}

void ThreadsManager::SetEndCallback(void (*fEndCb)()) {
    ThreadEndCallback = fEndCb;
}

DWORD WINAPI ThreadsManager::Routine(LPVOID params) {
    struct ManagedThread *pParams = (struct ManagedThread*) params;
    ThreadsManager *this_ = pParams->m_pManager;

    do {
        SetThreadAffinityMask(GetCurrentThread(), 1 << pParams->m_iTargetCpu);
        Sleep(10);
    } while (pParams->m_iTargetCpu != GetCurrentProcessorNumber());

    SetEvent(pParams->m_hReadyEvent);

    {
        std::unique_lock<std::mutex> l(this_->lock);
        while (!this_->bThreadsStarted) {
            this_->cv_start.wait(l);
        }
    }

    //int j = 0;
    //while (1000 > j) {
    //    {
    //        std::lock_guard<std::mutex> io_lock(this_->io_mutex);
    //        printf_s("Thread on core %d is working %d\n", pParams->m_iTargetCpu, j);
    //    }
    //    ++j;
    //}

    this_->RoutineFunc();
    return 0;
}

void ThreadsManager::CreateThreads(int threads_count, bool distinct_cpu) {

    int max_cpus = GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
    if (distinct_cpu && threads_count > max_cpus) {
        //threads_count = max_cpus;
        distinct_cpu = false;
    }

    for (int i = 0; i < threads_count; ++i) {
        int target = distinct_cpu ? i : 0;
        struct ManagedThread *pmt = new struct ManagedThread(this, target); //they will be deleted at ~ThreadsManager
        pthreads.push_back(pmt);

        DWORD threadId;
        HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThreadsManager::Routine, (LPVOID) pmt, 0, &threadId);

        if (!thread) {
            printf_s("Error: cannot create thread\n");
            return;
        }
        pmt->m_hThread = thread;
    }

    DWORD eventsCount = pthreads.size();
    HANDLE *pReadyEvents = new HANDLE[eventsCount];
    int i = 0;
    for (auto &t: pthreads) {
        pReadyEvents[i] = t->m_hReadyEvent;
        ++i;
    }

    WaitForMultipleObjects(eventsCount, pReadyEvents, TRUE, INFINITE);

    delete []pReadyEvents;
    ThreadsReadyCallback();
}

void ThreadsManager::StartThreads() {
    bThreadsStarted = true;
    cv_start.notify_all();

    DWORD threadsCount = pthreads.size();
    HANDLE *threads = new HANDLE[threadsCount];
    int i = 0;
    for (auto &t: pthreads) {
        threads[i] = t->m_hThread;
        ++i;
    }

    WaitForMultipleObjects(threadsCount, threads, TRUE, INFINITE);
    delete []threads;
    ThreadEndCallback();
}

ManagedThread::ManagedThread(ThreadsManager *pManager, int iTargetCpu):
    m_iTargetCpu(iTargetCpu),
    m_hThread(NULL),
    m_pManager(pManager)
{
    m_hReadyEvent = CreateEvent(
        NULL,   //default security attributes
        TRUE,   //manual-reset event
        FALSE,  //initial state is non-signalled
        NULL    //object name
        );
}

ManagedThread::~ManagedThread() {
    SAFE_CLOSE_HANDLE(m_hReadyEvent);
    SAFE_CLOSE_HANDLE(m_hThread);
}