/**
 * @file
 *
 * Define a class that abstracts Linux threads.
 */

/******************************************************************************
 * Copyright 2009-2011, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/
#include <qcc/platform.h>

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <map>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>

#include <Status.h>

using namespace std;

/** @internal */
#define QCC_MODULE "THREAD"

namespace qcc {


static uint32_t started = 0;
static uint32_t running = 0;
static uint32_t joined = 0;

/** Mutex that protects global thread list */
Thread::ThreadListLock Thread::threadListLock;
Mutex* Thread::ThreadListLock::m_mutex = NULL;
bool Thread::ThreadListLock::m_destructed = false;

/** Global thread list */
map<ThreadHandle, Thread*> Thread::threadList;

void Thread::SigHandler(int signal)
{
}

QStatus Sleep(uint32_t ms) {
    usleep(1000 * ms);
    return ER_OK;
}

Thread* Thread::GetThread()
{
    Thread* ret = NULL;

    /* Find thread on Thread::threadList */
    threadListLock.Lock();
    map<ThreadHandle, Thread*>::const_iterator iter = threadList.find(pthread_self());
    if (iter != threadList.end()) {
        ret = iter->second;
    }
    threadListLock.Unlock();

    /* If the current thread isn't on the list, then create an external (wrapper) thread */
    if (NULL == ret) {
        ret = new Thread("external", NULL, true);
    }

    return ret;
}

const char* Thread::GetThreadName()
{
    Thread*thread = NULL;

    /* Find thread on Thread::threadList */
    threadListLock.Lock();
    map<ThreadHandle, Thread*>::const_iterator iter = threadList.find(pthread_self());
    if (iter != threadList.end()) {
        thread = iter->second;
    }
    threadListLock.Unlock();

    /* If the current thread isn't on the list, then don't create an external (wrapper) thread */
    if (thread == NULL) {
        return "external";
    }

    return thread->GetName();
}

void Thread::CleanExternalThreads()
{
    threadListLock.Lock();
    map<ThreadHandle, Thread*>::iterator it = threadList.begin();
    while (it != threadList.end()) {
        if (it->second->isExternal) {
            delete it->second;
            threadList.erase(it++);
        } else {
            ++it;
        }
    }
    threadListLock.Unlock();
}

Thread::Thread(qcc::String name, Thread::ThreadFunction func, bool isExternal) :
#ifndef NDEBUG
    lockTrace(this),
#endif
    stopEvent(),
    state(isExternal ? RUNNING : INITIAL),
    isStopping(false),
    function(isExternal ? NULL : func),
    handle(isExternal ? pthread_self() : 0),
    exitValue(NULL),
    listener(NULL),
    isExternal(isExternal),
    alertCode(0),
    auxListeners(),
    auxListenersLock(),
    waitCount(0),
    waitLock(),
    hasBeenJoined(false)
{
    /* qcc::String is not thread safe.  Don't use it here. */
    funcName[0] = '\0';
    strncpy(funcName, name.c_str(), sizeof(funcName));
    funcName[sizeof(funcName) - 1] = '\0';

    /* If this is an external thread, add it to the thread list here since Run will not be called */
    if (isExternal) {
        assert(func == NULL);
        threadListLock.Lock();
        threadList[handle] = this;
        threadListLock.Unlock();
    }
    QCC_DbgHLPrintf(("Thread::Thread() created %s - %x -- started:%d running:%d joined:%d", funcName, handle, started, running, joined));
}


Thread::~Thread(void)
{
    QCC_DbgHLPrintf(("Thread::~Thread() destroying %s - %x", funcName, handle));

    if (!isExternal) {
        Stop();
        Join();
    }

    /* Keep object alive until waitCount goes to zero */
    while (waitCount) {
        qcc::Sleep(2);
    }

    QCC_DbgHLPrintf(("Thread::~Thread() destroyed %s - %x -- started:%d running:%d joined:%d", funcName, handle, started, running, joined));
}


ThreadInternalReturn Thread::RunInternal(void* threadArg)
{
    Thread* thread(reinterpret_cast<Thread*>(threadArg));
    sigset_t newmask;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);

    assert(thread != NULL);

    if (thread->state != STARTED) {
        return NULL;
    }

    /* Plug race condition between Start and Run. (pthread_create may not write handle before run is called) */
    thread->handle = pthread_self();

    ++started;

    QCC_DbgPrintf(("Thread::RunInternal: %s (pid=%x)", thread->funcName, (unsigned long) thread->handle));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &SigHandler;
    sa.sa_flags = 0;
    int ret = sigaction(SIGUSR1, &sa, NULL);
    if (0 != ret) {
        QCC_LogError(ER_OS_ERROR, ("Thread:Run() [%s] Failed to set SIGUSR1 handler", thread->funcName));
        thread->exitValue = reinterpret_cast<ThreadInternalReturn>(0);
    } else {

        /* Add this Thread to list of running threads */
        threadListLock.Lock();
        threadList[thread->handle] = thread;
        thread->state = RUNNING;
        pthread_sigmask(SIG_UNBLOCK, &newmask, NULL);
        threadListLock.Unlock();

        /* Start the thread if it hasn't been stopped */
        if (!thread->isStopping) {
            QCC_DbgPrintf(("Starting thread: %s", thread->funcName));
            ++running;
            thread->exitValue = thread->Run(thread->arg);
            --running;
            QCC_DbgPrintf(("Thread function exited: %s --> %p", thread->funcName, thread->exitValue));
        }
    }

    thread->state = STOPPING;
    thread->stopEvent.ResetEvent();

    /*
     * Call thread exit callback if specified. Note that ThreadExit may dellocate the thread so the
     * members of thread may not be accessed after this call
     */
    void* retVal = thread->exitValue;
    ThreadHandle handle = thread->handle;


    /* Call aux listeners before main listener since main listner may delete the thread */
    thread->auxListenersLock.Lock();
    ThreadListeners::iterator it = thread->auxListeners.begin();
    while (it != thread->auxListeners.end()) {
        ThreadListener* listener = *it;
        listener->ThreadExit(thread);
        it = thread->auxListeners.upper_bound(listener);
    }
    thread->auxListenersLock.Unlock();

    if (thread->listener) {
        thread->listener->ThreadExit(thread);
    }

    /* This also means no QCC_DbgPrintf as they try to get context on the current thread */

    if (ret == 0) {
        /* Remove this Thread from list of running threads */
        threadListLock.Lock();
        threadList.erase(handle);
        threadListLock.Unlock();
    }

    return reinterpret_cast<ThreadInternalReturn>(retVal);
}

static const uint32_t stacksize = 80 * 1024;

QStatus Thread::Start(void* arg, ThreadListener* listener)
{
    QStatus status = ER_OK;

    /* Check that thread can be started */
    if (isExternal) {
        status = ER_EXTERNAL_THREAD;
    } else if (isStopping) {
        status = ER_THREAD_STOPPING;
    } else if (IsRunning()) {
        status = ER_THREAD_RUNNING;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Thread::Start [%s]", funcName));
    } else {
        int ret;

        /* Clear/initialize the join context */
        hasBeenJoined = false;
        waitCount = 0;

        /*  Reset the stop event so the thread doesn't start out alerted. */
        stopEvent.ResetEvent();
        /* Create OS thread */
        this->arg = arg;
        this->listener = listener;

        state = STARTED;
        pthread_attr_t attr;
        ret = pthread_attr_init(&attr);
        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Initializing thread attr: %s", strerror(ret)));
        }
        ret = pthread_attr_setstacksize(&attr, stacksize);
        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting stack size: %s", strerror(ret)));
        }
        ret = pthread_create(&handle, &attr, RunInternal, this);
        QCC_DbgTrace(("Thread::Start() [%s] pid = %x", funcName, handle));
        if (ret != 0) {
            state = DEAD;
            isStopping = false;
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Creating thread %s: %s", funcName, strerror(ret)));
        }
    }
    return status;
}

QStatus Thread::Stop(void)
{
    /* Cannot stop external threads */
    if (isExternal) {
        QCC_LogError(ER_EXTERNAL_THREAD, ("Cannot stop an external thread"));
        return ER_EXTERNAL_THREAD;
    } else if ((state == DEAD) || (state == INITIAL)) {
        QCC_DbgPrintf(("Thread::Stop() thread is dead [%s]", funcName));
        return ER_OK;
    } else {
        QCC_DbgTrace(("Thread::Stop() %x [%s]", handle, funcName));
        isStopping = true;
        return stopEvent.SetEvent();
    }
}

QStatus Thread::Alert()
{
    if (state == DEAD) {
        return ER_DEAD_THREAD;
    }
    QCC_DbgTrace(("Thread::Alert() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

QStatus Thread::Alert(uint32_t alertCode)
{
    this->alertCode = alertCode;
    if (state == DEAD) {
        return ER_DEAD_THREAD;
    }
    QCC_DbgTrace(("Thread::Alert(%u) [%s:%srunning]", alertCode, funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

void Thread::AddAuxListener(ThreadListener* listener)
{
    auxListenersLock.Lock();
    auxListeners.insert(listener);
    auxListenersLock.Unlock();
}

void Thread::RemoveAuxListener(ThreadListener* listener)
{
    auxListenersLock.Lock();
    ThreadListeners::iterator it = auxListeners.find(listener);
    if (it != auxListeners.end()) {
        auxListeners.erase(it);
    }
    auxListenersLock.Unlock();
}

QStatus Thread::Join(void)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Thread::Join() [%s - %x :%srunning]", funcName, handle, IsRunning() ? " " : " not "));

    QCC_DbgPrintf(("[%s - %x] Joining thread [%s - %x]",
                   GetThread()->funcName, GetThread()->handle,
                   funcName, handle));

    /*
     * Nothing to join if the thread is dead
     */
    if (state == DEAD) {
        QCC_DbgPrintf(("Thread::Join() thread is dead [%s]", funcName));
        isStopping = false;
        return ER_OK;
    }
    /*
     * There is a race condition where the underlying OS thread has not yet started to run. We need
     * to wait until the thread is actually running before we can free it.
     */
    while (state == STARTED) {
        usleep(1000 * 5);
    }

    /* Threads that join themselves must detach without blocking */
    if (handle == pthread_self()) {
        int32_t waiters = IncrementAndFetch(&waitCount);
        if ((waiters == 1) && !hasBeenJoined) {
            hasBeenJoined = true;
            int ret = pthread_detach(handle);
            if (ret == 0) {
                ++joined;
            } else {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Detaching thread: %d - %s", ret, strerror(ret)));
            }
        }
        DecrementAndFetch(&waitCount);
        handle = 0;
        isStopping = false;
    }

    /*
     * Unfortunately, POSIX pthread_join can only be called once for a given thread. This is quite
     * inconvenient in a system of multiple threads that need to synchronize with each other.
     * This ugly looking code allows multiple threads to Join a thread. All but one thread block
     * in a Mutex. The first thread to obtain the mutex is the one that is allowed to call pthread_join.
     * All other threads wait for that join to complete. Then they are released.
     */
    if (handle) {
        int ret = 0;
        int32_t waiters = IncrementAndFetch(&waitCount);
        waitLock.Lock();
        if ((waiters == 1) && !hasBeenJoined) {
            hasBeenJoined = true;
            ret = pthread_join(handle, NULL);
            ++joined;
        }
        waitLock.Unlock();
        DecrementAndFetch(&waitCount);

        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Joining thread: %d - %s", ret, strerror(ret)));
        }
        handle = 0;
        isStopping = false;
    }
    state = DEAD;
    QCC_DbgPrintf(("Joined thread %s", funcName));
    return status;
}

ThreadReturn STDCALL Thread::Run(void* arg)
{
    QCC_DbgTrace(("Thread::Run() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    assert(NULL != function);
    assert(!isExternal);
    return (*function)(arg);
}

} /* namespace */
