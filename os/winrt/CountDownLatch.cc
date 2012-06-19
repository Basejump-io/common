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

#include <qcc/CountDownLatch.h>
#include <qcc/atomic.h>

/** @internal */
#define QCC_MODULE "COUNTDOWNLATCH"

using namespace qcc;

_CountDownLatch::_CountDownLatch() : _count(0)
{
    _evt.SetEvent();
}

_CountDownLatch::~_CountDownLatch()
{
}

QStatus _CountDownLatch::Wait(void)
{
    return qcc::Event::Wait(_evt, (uint32_t)-1);
}

int32_t _CountDownLatch::Current(void)
{
    return _count;
}

int32_t _CountDownLatch::Increment(void)
{
    int32_t val = qcc::IncrementAndFetch(&_count);
    if (val == 1) {
        // 0 -> 1
        _evt.ResetEvent();
    }
    return val;
}

int32_t _CountDownLatch::Decrement(void)
{
    int32_t val = qcc::DecrementAndFetch(&_count);
    if (val == 0) {
        // 1 -> 0
        _evt.SetEvent();
    }
    return val;
}
