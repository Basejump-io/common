/**
 * @file
 *
 * Define a class that abstracts WinRT mutexes.
 */

/******************************************************************************
 *
 * Copyright 2011-2012, Qualcomm Innovation Center, Inc.
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
 *
 *****************************************************************************/

#include <qcc/platform.h>

#include <windows.h>
#include <stdio.h>

#include <qcc/Mutex.h>

/** @internal */
#define QCC_MODULE "MUTEX"

using namespace qcc;


void Mutex::Init()
{
    if (!initialized && InitializeCriticalSectionEx(&mutex, 100, 0)) {
        initialized = true;
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
    return Lock();
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
    return Unlock();
}

bool Mutex::TryLock(void)
{
    if (!initialized) {
        return false;
    }
    return TryEnterCriticalSection(&mutex);
}
