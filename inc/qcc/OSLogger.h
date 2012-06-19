/**
 * @file
 *
 * OS dependent logging.
 */

/******************************************************************************
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
 ******************************************************************************/
#ifndef _QCC_OS_LOGGER_H
#define _QCC_OS_LOGGER_H

#include <qcc/platform.h>
#include <qcc/Debug.h>

/**
 * Get the OS specific logger if there is one.
 *
 * @param useOSLog
 *
 * @return  Returns a function pointer to the OS-specific logger.
 */
QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog);


#endif
