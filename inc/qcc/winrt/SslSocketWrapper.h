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

#pragma once

#include <qcc/winrt/SocketWrapperTypes.h>
#include <qcc/winrt/SocketWrapper.h>

namespace qcc {
namespace winrt {


#ifdef COMMON_WINRT_PUBLIC
public
#else
private
#endif
ref class SslSocketWrapper sealed {
  public:
    SslSocketWrapper();
    virtual ~SslSocketWrapper();

    uint32_t Init(AddressFamily addrFamily, SocketType type);
    uint32_t Connect(Platform::String ^ remoteAddr, int remotePort);
    uint32_t Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent);
    uint32_t Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received);
    uint32_t Close();
    uint32_t Shutdown();

    property uint32_t LastError;

  private:
    SocketWrapper ^ _sw;
};

}
}
// SocketWrapper.h
