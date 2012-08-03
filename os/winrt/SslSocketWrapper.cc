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

#include <qcc/IPAddress.h>
#include <qcc/Debug.h>

#include <qcc/winrt/utility.h>
#include <qcc/winrt/SslSocketWrapper.h>


using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SSL_SOCKET_WRAPPER"

namespace qcc {
namespace winrt {


SslSocketWrapper::SslSocketWrapper()
{
    LastError = ER_OK;
    _sw = ref new SocketWrapper();
}

SslSocketWrapper::~SslSocketWrapper()
{
    Close();
}

uint32_t SslSocketWrapper::Init(AddressFamily addrFamily, SocketType type)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Init(addrFamily, type);

    if (result == ER_OK) {
        _sw->Ssl = true;
    }

    SetLastError(result);
    return result;
}


uint32_t SslSocketWrapper::Connect(Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Connect(remoteAddr, remotePort);
    SetLastError(result);
    return result;
}


uint32_t SslSocketWrapper::Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Send(buf, len, sent);
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Recv(buf, len, received);
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Close()
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Close();
    SetLastError(result);
    return result;
}

uint32_t SslSocketWrapper::Shutdown()
{
    ::QStatus result = ER_FAIL;
    result = (::QStatus)_sw->Shutdown();
    SetLastError(result);
    return result;
}

}
}
