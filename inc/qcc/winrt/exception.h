/**
 * @file
 *
 * Define exception utility functions for WinRT.
 */

/******************************************************************************
 *
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
 ******************************************************************************/
#ifndef _OS_QCC_EXCEPTION_H
#define _OS_QCC_EXCEPTION_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>

namespace qcc {
extern const char* QCC_StatusMessage(uint32_t status);
}

// WinRT doesn't like exceptions without the S bit being set to 1 (failure)
#define QCC_THROW_EXCEPTION(e)                                                                                          \
    do {                                                                                                                \
        qcc::String message = "AllJoyn Error : ";                                                                       \
        message +=  qcc::QCC_StatusMessage(e);                                                                               \
        throw Platform::Exception::CreateException(0xC0000000 | (int)(e), MultibyteToPlatformString(message.c_str()));  \
    } while (0)


#endif
