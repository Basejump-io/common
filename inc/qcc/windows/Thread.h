/**
 * @file
 *
 * This file just wraps platform specific header files that define the thread
 * abstraction interface.
 */

/******************************************************************************
 *
 *
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
#ifndef _OS_QCC_THREAD_H
#define _OS_QCC_THREAD_H

#include <qcc/platform.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

/**
 * Windows uses __stdcall for certain API calls
 */
#define STDCALL __stdcall

/**
 * Redefine Windows' Sleep function to match Linux.
 *
 * @param x     Number of seconds to sleep.
 */
#define sleep(x) Sleep(x)

typedef HANDLE ThreadHandle;            ///< Window process handle typedef.

/**
 * Windows' thread function return type
 */
typedef unsigned int ThreadInternalReturn;


#endif
