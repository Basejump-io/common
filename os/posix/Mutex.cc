/**
 * @file
 *
 * Define a class that abstracts Linux mutex's.
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

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <qcc/Mutex.h>
#include <qcc/StringUtil.h>

#include <Status.h>

using namespace qcc;

void Mutex::Init()
{
    isInitialized = false;
    int ret;
    pthread_mutexattr_t attr;
    ret = pthread_mutexattr_init(&attr);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex attribute initialization failure: %d - %s\n", ret, strerror(ret));
        goto cleanup;
    }
    // We want entities to be able to lock a mutex multiple times without deadlocking or reporting an error.
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    ret = pthread_mutex_init(&mutex, &attr);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex initialization failure: %d - %s\n", ret, strerror(ret));
        goto cleanup;
    }

    isInitialized = true;

cleanup:
    // Don't need the attribute once it has been assigned to a mutex.
    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex()
{
    if (!isInitialized) {
        return;
    }
    int ret;
    ret = pthread_mutex_destroy(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexs under the hood.
        printf("***** Mutex destruction failure: %d - %s\n", ret, strerror(ret));
        assert(false);
    }
}

QStatus Mutex::Lock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_mutex_lock(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexes under the hood.
        printf("***** Mutex lock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

QStatus Mutex::Unlock()
{
    if (!isInitialized) {
        return ER_INIT_FAILED;
    }

    int ret = pthread_mutex_unlock(&mutex);
    if (ret != 0) {
        fflush(stdout);
        // Can't use ER_LogError() since it uses mutexes under the hood.
        printf("***** Mutex unlock failure: %d - %s\n", ret, strerror(ret));
        assert(false);
        return ER_OS_ERROR;
    }
    return ER_OK;
}

