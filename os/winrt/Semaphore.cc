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

#include <qcc/Semaphore.h>

/** @internal */
#define QCC_MODULE "SEMAPHORE"

using namespace qcc;

Semaphore::Semaphore() : _initialized(false), _semaphore(INVALID_HANDLE_VALUE), _initial(-1),  _maximum(-1)
{
}

Semaphore::~Semaphore()
{
    Close();
}

void Semaphore::Close()
{
    if (_initialized) {
        _initialized = false;
        if (INVALID_HANDLE_VALUE != _semaphore) {
            CloseHandle(_semaphore);
            _semaphore = INVALID_HANDLE_VALUE;
        }
    }
}

QStatus Semaphore::Init(int32_t initial, int32_t maximum)
{
    QStatus result = ER_FAIL;

    while (true) {
        if (!_initialized) {
            _initial = initial;
            _maximum = maximum;
            _semaphore = CreateSemaphoreEx(NULL, initial, maximum,
                                           NULL, 0, SEMAPHORE_ALL_ACCESS);
            if (NULL == _semaphore) {
                result = ER_OS_ERROR;
                break;
            }
            result = ER_OK;
            _initialized = true;
        }

        break;
    }

    return result;
}

QStatus Semaphore::Wait(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }
    return (WaitForSingleObjectEx(_semaphore, INFINITE, TRUE) == WAIT_OBJECT_0) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::Release(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }
    return (ReleaseSemaphore(_semaphore, 1, NULL) == TRUE) ? ER_OK : ER_FAIL;
}

QStatus Semaphore::Reset(void)
{
    if (!_initialized) {
        return ER_INIT_FAILED;
    }

    Close();

    return Init(_initial, _maximum);
}
