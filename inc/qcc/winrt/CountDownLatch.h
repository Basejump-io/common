/**
 * @file
 *
 * Define a class that abstracts WinRT count down latches.
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
#ifndef _OS__QCC_COUNTDOWNLATCH_H
#define _OS__QCC_COUNTDOWNLATCH_H

#include <windows.h>
#include <qcc/platform.h>
#include <qcc/Event.h>
#include <qcc/ManagedObj.h>
#include <Status.h>

namespace qcc {

/**
 * The WinRT implementation of a count down latch abstraction class.
 */
class _CountDownLatch {
  public:

    /**
     * Constructor
     */
    _CountDownLatch();

    /**
     * Destructor
     */
    ~_CountDownLatch();

    /**
     * Blocks the currently executing thread while the latch count
     * is non-zero.
     */
    QStatus Wait(void);

    /**
     * Gets the current latch count
     *
     * @return  The current latch count.
     */
    int32_t Current(void);

    /**
     * Gets the current latch count
     *
     * @return  The post incremented latch count.
     */
    int32_t Increment(void);

    /**
     * Gets the current latch count
     *
     * @return  The post decremeneted latch count.
     */
    int32_t Decrement(void);

  private:
    volatile int32_t _count;
    qcc::Event _evt;
};

/**
 * CountDownLatch is a reference counted (managed) version of _CountDownLatch
 */
typedef qcc::ManagedObj<_CountDownLatch> CountDownLatch;

} /* namespace */

#endif
