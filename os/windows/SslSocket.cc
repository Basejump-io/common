/**
 * @file
 *
 * SSL stream-based socket implementation for Windows
 *
 */

/******************************************************************************
 * Copyright 2012 Qualcomm Innovation Center, Inc.
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
#include <qcc/IPAddress.h>
#include <qcc/SslSocket.h>


#define QCC_MODULE  "SSL"


namespace qcc {

SslSocket::SslSocket(String host) :
    internal(NULL),
    sourceEvent(&qcc::Event::neverSet),
    sinkEvent(&qcc::Event::neverSet),
    Host(host),
    sock(-1)
{
}

SslSocket::~SslSocket() {

}

QStatus SslSocket::Connect(const qcc::String hostName, uint16_t port)
{
    return ER_FAIL;
}

void SslSocket::Close()
{
}

QStatus SslSocket::PullBytes(void*buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    return ER_FAIL;
}

QStatus SslSocket::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    return ER_FAIL;
}

QStatus SslSocket::ImportPEM()
{
    return ER_FAIL;
}

}  /* namespace */
