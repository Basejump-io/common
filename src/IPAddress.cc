/**
 * @file
 *
 * This file implements methods from IPAddress.h.
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

#include <qcc/platform.h>

#ifdef QCC_OS_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <algorithm>
#include <ctype.h>
#include <string.h>

#include <qcc/Debug.h>
#include <qcc/IPAddress.h>
#include <qcc/SocketTypes.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <Status.h>

#define QCC_MODULE "NETWORK"

using namespace std;
using namespace qcc;

IPAddress::IPAddress(const uint8_t* addrBuf, size_t addrBufSize)
{
    assert(addrBuf != NULL);
    assert(addrBufSize == IPv4_SIZE || addrBufSize == IPv6_SIZE);
    addrSize = (uint16_t)addrBufSize;
    if (addrSize == IPv4_SIZE) {
        // Encode the IPv4 address in the IPv6 address space for easy
        // conversion.
        memset(addr, 0, sizeof(addr) - 6);
        memset(&addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t)], 0xff, sizeof(uint16_t));
    }
    memcpy(&addr[IPv6_SIZE - addrSize], addrBuf, addrSize);
}

IPAddress::IPAddress(uint32_t ipv4Addr)
{
    addrSize = IPv4_SIZE;
    memset(addr, 0, sizeof(addr) - 6);
    addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t) + 0] = 0xff;
    addr[IPv6_SIZE - IPv4_SIZE - sizeof(uint16_t) + 1] = 0xff;
    addr[IPv6_SIZE - IPv4_SIZE + 0] = (uint8_t)((ipv4Addr >> 24) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 1] = (uint8_t)((ipv4Addr >> 16) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 2] = (uint8_t)((ipv4Addr >> 8) & 0xff);
    addr[IPv6_SIZE - IPv4_SIZE + 3] = (uint8_t)(ipv4Addr & 0xff);
}

qcc::String IPAddress::IPv4ToString(const uint8_t addr[])
{
    qcc::String oss;
    size_t pos;

    pos = 0;
    oss.append(U32ToString(static_cast<uint32_t>(addr[pos]), 10));;
    for (++pos; pos < IPv4_SIZE; ++pos) {
        oss.push_back('.');
        oss.append(U32ToString(static_cast<uint32_t>(addr[pos]), 10));
    }

    return oss;
}

qcc::String IPAddress::IPv6ToString(const uint8_t addr[])
{
    qcc::String oss;
    size_t i;
    size_t j;
    int zerocnt;
    int maxzerocnt = 0;

    for (i = 0; i < IPv6_SIZE; i += 2) {
        if (addr[i] == 0 && addr[i + 1] == 0) {
            zerocnt = 0;
            for (j = IPv6_SIZE - 2; j > i; j -= 2) {
                if (addr[j] == 0 && addr[j + 1] == 0) {
                    ++zerocnt;
                    maxzerocnt = max(maxzerocnt, zerocnt);
                } else {
                    zerocnt = 0;
                }
            }
            // Count the zero we are pointing to.
            ++zerocnt;
            maxzerocnt = max(maxzerocnt, zerocnt);

            if (zerocnt == maxzerocnt) {
                oss.push_back(':');
                if (i == 0) {
                    oss.push_back(':');
                }
                i += (zerocnt - 1) * 2;
                continue;
            }
        }
        oss.append(U32ToString((uint32_t)(addr[i] << 8 | addr[i + 1]), 16));
        if (i + 2 < IPv6_SIZE) {
            oss.push_back(':');
        }
    }
    return oss;
}

IPAddress::IPAddress(const qcc::String& addrString)
{
    QStatus status = SetAddress(addrString, false);
    if (ER_OK != status) {
        QCC_LogError(status, ("Could not resolve \"%s\". Defaulting to INADDR_ANY", addrString.c_str()));
        SetAddress("");
    }
}

// A liberal interpretation of ADDR_ANY
static bool AddrAny(const qcc::String& addr, char delim)
{
    for (size_t i = 0; i < addr.size(); ++i) {
        if ((addr[i] != '0') && (addr[i] != delim)) {
            return false;
        }
    }
    return true;
}

QStatus IPAddress::SetAddress(const qcc::String& addrString, bool allowHostNames)
{
    QStatus status = ER_OK;

    addrSize = 0;
    memset(addr, 0xFF, sizeof(addr));

    if (addrString.empty()) {
        // INADDR_ANY
        addrSize = IPv4_SIZE;
        memcpy(&addr, &in6addr_any, sizeof(in6addr_any));
    } else if (addrString.find_first_of(':') != addrString.npos) {
        // IPV6
        if (INET_PTON(AF_INET6, addrString.c_str(), addr) >= 0) {
            addrSize = IPv6_SIZE;
        } else if (AddrAny(addrString, ':')) {
            addrSize = IPv6_SIZE;
            memcpy(&addr, &in6addr_any, sizeof(in6addr_any));
        } else {
            status = ER_PARSE_ERROR;
        }
    } else if (isdigit(addrString[0])) {
        // IPV4
        if (INET_PTON(AF_INET, addrString.c_str(), &addr[IPv6_SIZE - IPv4_SIZE]) >= 0) {
            addrSize = IPv4_SIZE;
        } else if (AddrAny(addrString, '.')) {
            addrSize = IPv4_SIZE;
            memcpy(&addr, &in6addr_any, sizeof(in6addr_any));
        } else {
            status = ER_PARSE_ERROR;
        }
    } else if (allowHostNames) {
        // Hostname
        struct addrinfo* info;
        if (0 == getaddrinfo(addrString.c_str(), NULL, NULL, &info)) {
            if (info->ai_family == AF_INET6) {
                struct sockaddr_in6* sa = (struct sockaddr_in6*) info->ai_addr;
                memcpy(addr, &sa->sin6_addr, IPv6_SIZE);
                addrSize = IPv6_SIZE;
            } else if (info->ai_family == AF_INET) {
                struct sockaddr_in* sa = (struct sockaddr_in*) info->ai_addr;
                memcpy(&addr[IPv6_SIZE - IPv4_SIZE], &sa->sin_addr, IPv4_SIZE);
                addrSize = IPv4_SIZE;
            } else {
                status = ER_FAIL;
            }
        } else {
            status = ER_BAD_HOSTNAME;
        }
    } else {
        status = ER_PARSE_ERROR;
    }
    return status;
}

QStatus IPAddress::RenderIPv4Binary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    assert(addrSize == IPv4_SIZE);
    if (addrBufSize < IPv4_SIZE) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IPv4 address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, &addr[IPv6_SIZE - IPv4_SIZE], IPv4_SIZE);

exit:
    return status;
}
QStatus IPAddress::RenderIPv6Binary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    assert(addrSize == IPv6_SIZE);
    if (addrBufSize < IPv6_SIZE) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IPv6 address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, addr, IPv6_SIZE);

exit:
    return status;
}

QStatus IPAddress::RenderIPBinary(uint8_t addrBuf[], size_t addrBufSize) const
{
    QStatus status = ER_OK;
    if (addrBufSize < addrSize) {
        status = ER_BUFFER_TOO_SMALL;
        QCC_LogError(status, ("Copying IP address to buffer"));
        goto exit;
    }
    memcpy(addrBuf, &addr[IPv6_SIZE - addrSize], addrSize);

exit:
    return status;
}


uint32_t IPAddress::GetIPv4AddressCPUOrder(void) const
{
    return ((static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 0]) << 24) |
            (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 1]) << 16) |
            (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 2]) << 8) |
            static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 3]));
}

uint32_t IPAddress::GetIPv4AddressNetOrder(void) const
{
    uint32_t addr4;
    memcpy(&addr4, &addr[IPv6_SIZE - IPv4_SIZE], IPv4_SIZE);
    return addr4;
}
