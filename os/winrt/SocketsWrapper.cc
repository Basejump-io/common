/******************************************************************************
 *
 * Copyright 2011-2012, Qualcomm Innovation Center, Inc.
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

#include <qcc/winrt/SocketsWrapper.h>
#include <qcc/IPAddress.h>
#include <qcc/Mutex.h>
#include <map>

using namespace Windows::Foundation;

namespace qcc {
namespace winrt {

static qcc::Mutex _fdMapMutex;
static std::map<void*, int> _fdMap;

#define ADJUST_BAD_ARGUMENT_DOMAIN(arg) \
    if ((int)arg >= (int)ER_BAD_ARG_1 && (int)arg <= (int)ER_BAD_ARG_8) { \
        arg = (::QStatus)((int)arg + 1); \
    }

static inline void AddObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    pUnk->__abi_AddRef();
}

static inline void RemoveObjectReference(Platform::Object ^ obj)
{
    __abi_IUnknown* pUnk = reinterpret_cast<__abi_IUnknown*>(obj);
    pUnk->__abi_Release();
}

uint32_t SocketsWrapper::Socket(AddressFamily addrFamily, SocketType type, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket || socket->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        qcc::winrt::SocketWrapper ^ sock = ref new qcc::winrt::SocketWrapper();
        if (nullptr == sock) {
            status = ER_OS_ERROR;
            break;
        }

        status = (::QStatus)sock->Init(addrFamily, type);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK == status) {
            socket[0] = sock;
        }
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SocketDup(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ dupSocket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == dupSocket || dupSocket->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        status = (::QStatus)socket->SocketDup(dupSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName)
{
    ::QStatus status = ER_NOT_IMPLEMENTED;

    return status;
}

uint32_t SocketsWrapper::Bind(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ name, int localPort)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->Bind(name, localPort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Listen(qcc::winrt::SocketWrapper ^ socket, int backlog)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->Listen(backlog);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        if (nullptr == remotePort || remotePort->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        if (nullptr == newSocket || newSocket->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        status = (::QStatus)socket->Accept(remoteAddr, remotePort, newSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Accept(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ newSocket)
{
    ::QStatus status = ER_FAIL;
    Platform::Array<Platform::String ^> ^ remoteAddr = ref new Platform::Array<Platform::String ^>(1);
    Platform::Array<int> ^ remotePort = ref new Platform::Array<int>(1);

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == newSocket || newSocket->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        status = (::QStatus)socket->Accept(remoteAddr, remotePort, newSocket);
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SetBlocking(qcc::winrt::SocketWrapper ^ socket, bool blocking)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->SetBlocking(blocking);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SetNagle(qcc::winrt::SocketWrapper ^ socket, bool useNagle)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->SetNagle(useNagle);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ pathName)
{
    ::QStatus status = ER_NOT_IMPLEMENTED;

    return status;
}

uint32_t SocketsWrapper::Connect(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->Connect(remoteAddr, remotePort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SendTo(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ remoteAddr, int remotePort,
                                const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == sent || sent->Length != 1) {
            status = ER_BAD_ARG_6;
            break;
        }

        status = (::QStatus)socket->SendTo(remoteAddr, remotePort, buf, len, sent);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::RecvFrom(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                                  Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == received || received->Length != 1) {
            status = ER_BAD_ARG_6;
            break;
        }

        status = (::QStatus)socket->RecvFrom(remoteAddr, remotePort, buf, len, received);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Send(qcc::winrt::SocketWrapper ^ socket, const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == sent || sent->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        status = (::QStatus)socket->Send(buf, len, sent);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Recv(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == received || received->Length != 1) {
            status = ER_BAD_ARG_4;
            break;
        }

        status = (::QStatus)socket->Recv(buf, len, received);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::GetLocalAddress(qcc::winrt::SocketWrapper ^ socket, Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        if (nullptr == addr || addr->Length != 1) {
            status = ER_BAD_ARG_2;
            break;
        }

        if (nullptr == port || port->Length != 1) {
            status = ER_BAD_ARG_3;
            break;
        }

        status = (::QStatus)socket->GetLocalAddress(addr, port);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Close(qcc::winrt::SocketWrapper ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->Close();
        break;
    }

    return status;
}

uint32_t SocketsWrapper::Shutdown(qcc::winrt::SocketWrapper ^ socket)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->Shutdown();
        break;
    }

    return status;
}

uint32_t SocketsWrapper::JoinMulticastGroup(qcc::winrt::SocketWrapper ^ socket, Platform::String ^ host)
{
    ::QStatus status = ER_FAIL;

    while (true) {
        if (nullptr == socket) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)socket->JoinMulticastGroup(host);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        break;
    }

    return status;
}

uint32_t SocketsWrapper::SocketPair(Platform::WriteOnlyArray<qcc::winrt::SocketWrapper ^> ^ sockets)
{
    ::QStatus status = ER_FAIL;
    Platform::String ^ ipAddr("127.0.0.1");
    Platform::Array<qcc::winrt::SocketWrapper ^> ^ refSocket = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(1);
    Platform::Array<qcc::winrt::SocketWrapper ^> ^ socketSet = ref new Platform::Array<qcc::winrt::SocketWrapper ^>(2);

    while (true) {
        if (nullptr == sockets || sockets->Length != 2) {
            status = ER_BAD_ARG_1;
            break;
        }

        status = (::QStatus)Socket(AddressFamily::QCC_AF_INET, SocketType::QCC_SOCK_STREAM, refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        socketSet[0] = refSocket[0];

        status = (::QStatus)Socket(AddressFamily::QCC_AF_INET, SocketType::QCC_SOCK_STREAM, refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        socketSet[1] = refSocket[0];

        status = (::QStatus)Bind(socketSet[0], ipAddr, 0);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        status = (::QStatus)Listen(socketSet[0], 1);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        // Get the port dynamically allocated from bind
        Platform::Array<int> ^ localPort = ref new Platform::Array<int>(1);
        Platform::Array<Platform::String ^> ^ localAddr = ref new Platform::Array<Platform::String ^>(1);
        status = (::QStatus)GetLocalAddress(socketSet[0], localAddr, localPort);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        status = (::QStatus)Connect(socketSet[1], localAddr[0], localPort[0]);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        status = (::QStatus)Accept(socketSet[0], refSocket);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }
        socketSet[0]->Close();
        socketSet[0] = refSocket[0];

        // Make sockets blocking
        status = (::QStatus)SetBlocking(socketSet[0], true);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        status = (::QStatus)SetBlocking(socketSet[1], true);
        ADJUST_BAD_ARGUMENT_DOMAIN(status)
        if (ER_OK != status) {
            break;
        }

        sockets[0] = socketSet[0];
        sockets[1] = socketSet[1];
        break;
    }

    return status;
}

int SocketsWrapper::IncrementFDMap(qcc::winrt::SocketWrapper ^ socket)
{
    int count = 0;
    void* sockfd =  reinterpret_cast<void*>(socket);

    _fdMapMutex.Lock();

    if (_fdMap.find(sockfd) == _fdMap.end()) {
        // AddRef the socket to fixup ref counting in map
        AddObjectReference(socket);
        _fdMap[sockfd] = ++count;
    } else {
        count = ++_fdMap[sockfd];
    }

    _fdMapMutex.Unlock();

    return count;
}

int SocketsWrapper::DecrementFDMap(qcc::winrt::SocketWrapper ^ socket)
{
    int count = -1;
    void* sockfd =  reinterpret_cast<void*>(socket);

    _fdMapMutex.Lock();

    if (_fdMap.find(sockfd) != _fdMap.end()) {
        count = --_fdMap[sockfd];
    }

    if (count == 0) {
        // Release the socket to fixup ref counting in map
        RemoveObjectReference(socket);
        _fdMap.erase(sockfd);
    }

    _fdMapMutex.Unlock();

    return count;
}

}
}

