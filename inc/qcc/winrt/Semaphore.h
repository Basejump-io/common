/**
 * @file
 *
 * Define a class that abstracts WinRT semaphores.
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
#ifndef _OS_QCC_SEMAPHORE_H
#define _OS_QCC_SEMAPHORE_H

#include <windows.h>
#include <qcc/platform.h>
#include <Status.h>

namespace qcc {

/**
 * The Windows implementation of a Semaphore abstraction class.
 */
class Semaphore {
  public:

    /**
     * Constructor
     */
    Semaphore();

    /**
     * Destructor
     */
    ~Semaphore();

    /**
     * Terminates the semaphore and unblocks any waiters
     */
    void Close();

    /**
     * Initializes the default state of the semaphore.
     *
     * @return  ER_OK if the initialization was successful, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Init(int32_t initial, int32_t maximum);

    /**
     * Blocks the currently executing thread until a resource can be
     * acquired.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Wait(void);

    /**
     * Adds a resource to the semaphore. A blocking thread will return if
     * waiting on a resource.
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Release(void);

    /**
     * Resets the semaphore state to the default values specfied during Init()
     *
     * @return  ER_OK if the lock was acquired, ER_OS_ERROR if the underlying
     *          OS reports an error.
     */
    QStatus Reset(void);

  private:
    bool _initialized;
    int32_t _initial;
    int32_t _maximum;
    HANDLE _semaphore;
};

} /* namespace */

#endif
