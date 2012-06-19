/**
 * @file
 *
 * Defines the abstracted socket interface types for WinRT.
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

#pragma once

#include <qcc/platform.h>
#include "SocketTypes.h"

namespace qcc {

namespace winrt {

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
enum class AddressFamily {
    QCC_AF_UNSPEC = qcc::AddressFamily::QCC_AF_UNSPEC,
    QCC_AF_INET  = qcc::AddressFamily::QCC_AF_INET,
    QCC_AF_INET6 = qcc::AddressFamily::QCC_AF_INET6,
    QCC_AF_UNIX  = qcc::AddressFamily::QCC_AF_UNIX,
};

#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
enum class SocketType {
    QCC_SOCK_STREAM =    qcc::SocketType::QCC_SOCK_STREAM,
    QCC_SOCK_DGRAM =     qcc::SocketType::QCC_SOCK_DGRAM,
    QCC_SOCK_SEQPACKET = qcc::SocketType::QCC_SOCK_SEQPACKET,
    QCC_SOCK_RAW =       qcc::SocketType::QCC_SOCK_RAW,
    QCC_SOCK_RDM =       qcc::SocketType::QCC_SOCK_RDM,
};

}
}

