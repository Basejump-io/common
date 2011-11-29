/**
 * @file
 *
 * Define a class that abstracts Windows mutexs.
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

#include <windows.h>
#include <stdio.h>

#include <qcc/Thread.h>
#include <qcc/Mutex.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;

void Mutex::Init()
{
    if (InitializeCriticalSectionAndSpinCount(&mutex, 100)) {
        initialized = true;
    } else {
        char buf[80];
        uint32_t ret = GetLastError();
        strerror_r(ret, buf, sizeof(buf));
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("**** Mutex initialization failure: %u - %s", ret, buf);
    }

}

Mutex::~Mutex()
{
    if (initialized) {
        initialized = false;
        DeleteCriticalSection(&mutex);
    }
}

QStatus Mutex::Lock(void)
{
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    EnterCriticalSection(&mutex);
    return ER_OK;
}

QStatus Mutex::Lock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    return Lock();
#else
    QStatus status;
    if (TryLock()) {
        status = ER_OK;
    } else {
        Thread::GetThread()->lockTrace.Waiting(this, file, line);
        status = Lock();
    }
    Thread::GetThread()->lockTrace.Acquired(this, file, line);
    return status;
#endif
}

QStatus Mutex::Unlock(void)
{
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    LeaveCriticalSection(&mutex);
    return ER_OK;
}

QStatus Mutex::Unlock(const char* file, uint32_t line)
{
#ifdef NDEBUG
    return Unlock();
#else
    if (!initialized) {
        return ER_INIT_FAILED;
    }
    Thread::GetThread()->lockTrace.Releasing(this, file, line);
    return Unlock();
#endif
}

bool Mutex::TryLock(void)
{
    if (!initialized) {
        return false;
    }
    return TryEnterCriticalSection(&mutex);
}
