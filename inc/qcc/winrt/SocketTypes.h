/**
 * @file
 *
 * Define the abstracted socket interface for WinRT.
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
#ifndef _OS_QCC_SOCKETTYPES_H
#define _OS_QCC_SOCKETTYPES_H

#include <qcc/platform.h>

namespace qcc {

/**
 * Enumeration of address families.
 */
typedef enum {
    QCC_AF_UNSPEC = -1, /**< unspecified address family */
    QCC_AF_INET  = 1,  /**< IPv4 address family */
    QCC_AF_INET6 = 2, /**< IPv6 address family */
    QCC_AF_UNIX  = -1        /**< Not implemented on windows */
} AddressFamily;

/**
 * Enumeration of socket types.
 */
typedef enum {
    QCC_SOCK_STREAM =    1,    /**< TCP */
    QCC_SOCK_DGRAM =     2,     /**< UDP */
    QCC_SOCK_SEQPACKET = -1, /**< Sequenced data transmission */
    QCC_SOCK_RAW =       -1,       /**< Raw IP packet */
    QCC_SOCK_RDM =       -1        /**< Reliable datagram */
} SocketType;
}

#endif
