/**
 * @file
 *
 * Platform specific logger for posix platforms
 */

/******************************************************************************
 *
 * Copyright 2012, Qualcomm Innovation Center, Inc.
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

#include <qcc/OSLogger.h>

#if defined (QCC_OS_ANDROID)

#include <android/log.h>

static void AndroidLogCB(DbgMsgType type, const char* module, const char* msg, void* context)
{
    int priority;
    switch (type) {
    case DBG_LOCAL_ERROR:
    case DBG_REMOTE_ERROR:
        priority = ANDROID_LOG_ERROR;
        break;

    case DBG_HIGH_LEVEL:
        priority = ANDROID_LOG_WARN;
        break;

    case DBG_GEN_MESSAGE:
        priority = ANDROID_LOG_INFO;
        break;

    case DBG_API_TRACE:
        priority = ANDROID_LOG_DEBUG;
        break;

    default:
    case DBG_REMOTE_DATA:
    case DBG_LOCAL_DATA:
        priority = ANDROID_LOG_VERBOSE;
        break;
    }
    __android_log_write(priority, module, msg);
}

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    return useOSLog ? AndroidLogCB : NULL;
}

#else // plain posix

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    return NULL;
}

#endif
