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

#include <ctxtcall.h>
#include <ppltasks.h>
#include <algorithm>

#include <qcc/atomic.h>
#include <qcc/IPAddress.h>
#include <qcc/Debug.h>
#include <qcc/StringUtil.h>

#include <qcc/winrt/utility.h>
#include <qcc/winrt/SocketsWrapper.h>
#include <qcc/winrt/SocketWrapper.h>
#include <qcc/winrt/SocketWrapperEvents.h>

#define DEFAULT_READ_SIZE_BYTES 16384

using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SOCKET_WRAPPER"

namespace qcc {
namespace winrt {

static uint8_t _anyAddrIPV4[4] = { 0 };
static uint8_t _anyAddrIPV6[16] = { 0 };
static SocketWrapperEvents _socketEvents;

SocketWrapper::SocketWrapper() : _initialized(false), _blocking(true), _lastBindHostname(nullptr), _backlog(1),
    _lastBindPort(0), _tcpSocketListener(nullptr), _callbackCount(0), _bindingState(BindingState::None), _ssl(false),
    _udpSocket(nullptr), _tcpSocket(nullptr), _dataReader(nullptr), _lastConnectHostname(nullptr), _lastConnectPort(0),
    _events((int) Events::Write), _eventMask((int) Events::All)
{
    LastError = ER_OK;
}

SocketWrapper::~SocketWrapper()
{
    Close();
}

Platform::String ^ SocketWrapper::SanitizeAddress(Platform::String ^ hostName)
{
    uint32_t status = ER_FAIL;;
    Platform::String ^ result = hostName;
    while (true) {
        if (nullptr == hostName) {
            status = ER_OK;
            break;
        }
        qcc::String strHostName = PlatformToMultibyteString(hostName);
        if (strHostName.empty() && nullptr != hostName) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        if (_socketAddrFamily == AddressFamily::QCC_AF_INET) {
            uint8_t addrBuf[4];
            status = (uint32_t)(int)qcc::IPAddress::StringToIPv4(strHostName, addrBuf, sizeof(addrBuf));
            if (ER_OK == status) {
                if (memcmp(addrBuf, _anyAddrIPV4, sizeof(_anyAddrIPV4)) == 0) {
                    result = nullptr;
                    break;
                }
            }
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET6) {
            uint8_t addrBuf[16];
            status = (uint32_t)(int)qcc::IPAddress::StringToIPv6(strHostName, addrBuf, sizeof(addrBuf));
            if (ER_OK == status) {
                if (memcmp(addrBuf, _anyAddrIPV6, sizeof(_anyAddrIPV6)) == 0) {
                    result = nullptr;
                    break;
                }
            }
        }
        break;
    }
    SetLastError(status);
    return result;
}

uint32_t SocketWrapper::IsValidAddress(Platform::String ^ hostName)
{
    ::QStatus result = ER_FAIL;
    while (true) {
        if (nullptr == hostName) {
            // this is acceptable in both ipv4 & ipv6
            result = ER_OK;
            break;
        }
        qcc::String strHostName = PlatformToMultibyteString(hostName);
        if (strHostName.empty() && nullptr != hostName) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(hostName);
        if (nullptr == hostname) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        if ((_ssl == true) && (hostname->Type == Windows::Networking::HostNameType::DomainName)) {
            // SSL requires a string hostname to verify the server certificate is valid
            result = ER_OK;
            break;
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET) {
            uint8_t addrBuf[4];
            result = (::QStatus)qcc::IPAddress::StringToIPv4(strHostName, addrBuf, sizeof(addrBuf));
            break;
        } else if (_socketAddrFamily == AddressFamily::QCC_AF_INET6) {
            uint8_t addrBuf[16];
            result = (::QStatus)qcc::IPAddress::StringToIPv6(strHostName, addrBuf, sizeof(addrBuf));
            break;
        }
        break;
    }
    SetLastError(result);
    return result;
}

void SocketWrapper::Cleanup()
{
    ::QStatus result = ER_OK;
    while (_initialized) {
        _initialized = false;
        SetBindingState(BindingState::Exception);
        for (std::map<uint32, Windows::Storage::Streams::DataReaderLoadOperation ^>::iterator it = _receiveOperationsMap.begin();
             it != _receiveOperationsMap.end();
             ++it) {
            it->second->Cancel();
        }
        _receiveOperationsMap.clear();
        for (std::map<uint32, concurrency::task<void> >::iterator it = _receiveTasksMap.begin();
             it != _receiveTasksMap.end();
             ++it) {
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            _mutex.Unlock();
            try {
                task.wait();
            } catch (...) {
            }
            _mutex.Lock();
            if (_receiveTasksMap.find(id) != _receiveTasksMap.end()) {
                _receiveTasksMap.erase(id);
            }
            it = _receiveTasksMap.begin();
            if (it == _receiveTasksMap.end()) {
                break;
            }
        }
        _receiveTasksMap.clear();
        for (std::map<uint32, Windows::Foundation::IAsyncOperationWithProgress<uint32, uint32> ^>::iterator it = _sendOperationsMap.begin();
             it != _sendOperationsMap.end();
             ++it) {
            it->second->Cancel();
        }
        _sendOperationsMap.clear();
        for (std::map<uint32, concurrency::task<void> >::iterator it = _sendTasksMap.begin();
             it != _sendTasksMap.end();
             ++it) {
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            _mutex.Unlock();
            try {
                task.wait();
            } catch (...) {
            }
            _mutex.Lock();
            if (_sendTasksMap.find(id) != _sendTasksMap.end()) {
                _sendTasksMap.erase(id);
            }
            it = _sendTasksMap.begin();
            if (it == _sendTasksMap.end()) {
                break;
            }
        }
        _sendTasksMap.clear();
        for (std::map<uint32, Windows::Foundation::IAsyncAction ^>::iterator it = _connectOperationsMap.begin();
             it != _connectOperationsMap.end();
             ++it) {
            it->second->Cancel();
        }
        _connectOperationsMap.clear();
        for (std::map<uint32, concurrency::task<void> >::iterator it = _connectTasksMap.begin();
             it != _connectTasksMap.end();
             ++it) {
            concurrency::task<void> task = it->second;
            uint32 id = it->first;
            _mutex.Unlock();
            try {
                task.wait();
            } catch (...) {
            }
            _mutex.Lock();
            if (_connectTasksMap.find(id) != _connectTasksMap.end()) {
                _connectTasksMap.erase(id);
            }
            it = _connectTasksMap.begin();
            if (it == _connectTasksMap.end()) {
                break;
            }
        }
        _connectTasksMap.clear();
        qcc::Event waiter;
        _mutex.Unlock();
        while (_callbackCount != 0) {
            waiter.Wait(waiter, 10);
        }
        _mutex.Lock();
        if (nullptr != _tcpSocket) {
            delete _tcpSocket;
            _tcpSocket = nullptr;
        }
        if (nullptr != _tcpSocketListener) {
            delete _tcpSocketListener;
            _tcpSocketListener = nullptr;
        }
        if (nullptr != _udpSocket) {
            delete _udpSocket;
            _udpSocket = nullptr;
        }
        if (nullptr != _dataReader) {
            delete _dataReader;
            _dataReader = nullptr;
        }
        _semAcceptQueue.Close();
        _semReceiveDataQueue.Close();
        _lastBindHostname = nullptr;
        _lastBindPort = 0;
        _lastConnectHostname = nullptr;
        _lastConnectPort = 0;
        _tcpBacklog.clear();
        _udpBacklog.clear();
        break;
    }
    SetLastError(result);
}

uint32_t SocketWrapper::Init(StreamSocket ^ socket, DataReader ^ reader, AddressFamily addrFamily)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        result = (::QStatus)Init(addrFamily, SocketType::QCC_SOCK_STREAM);
        if (ER_OK != result) {
            break;
        }
        SetBindingState(BindingState::Connect);
        _dataReader = reader;
        _tcpSocket = socket;
        result = (::QStatus)QueueTraffic();
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Init(AddressFamily addrFamily, SocketType type)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            if ((int)addrFamily == -1) {
                result = ER_BAD_ARG_1;
                break;
            }
            if ((int)type == -1) {
                result = ER_BAD_ARG_2;
                break;
            }
            _socketAddrFamily = addrFamily;
            _socketType = type;
            result = (::QStatus)((int)_semAcceptQueue.Init(0, 0x7FFFFFFF));
            if (ER_OK != result) {
                break;
            }
            result = (::QStatus)((int)_semReceiveDataQueue.Init(0, 0x7FFFFFFF));
            if (ER_OK != result) {
                break;
            }
            qcc::winrt::SocketsWrapper::IncrementFDMap(this);
            SocketWrapperEventsChangedHandler ^ handler = ref new SocketWrapperEventsChangedHandler([ = ] (Platform::Object ^ source, int events) {
                                                                                                    });
            if (nullptr == handler) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            SocketEventsChanged::add(handler);
            result = ER_OK;
            _initialized = true;
        } else {
            result = ER_INIT_FAILED;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SocketDup(Platform::WriteOnlyArray<SocketWrapper ^> ^ dupSocket)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == dupSocket || dupSocket->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        dupSocket[0] = this;
        SocketsWrapper::IncrementFDMap(this);
        result = ER_OK;
        break;
    }
    if (ER_OK != result) {
        if (nullptr != dupSocket) {
            dupSocket[0] = nullptr;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Bind(Platform::String ^ bindName, int localPort)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
        {
            if (nullptr == _tcpSocketListener ||
                _lastBindHostname != bindName ||
                _lastBindPort != localPort) {
                result = (::QStatus)IsValidAddress(bindName);
                if (ER_OK != result) {
                    break;
                }
                _lastBindHostname = bindName;
                _lastBindPort = localPort;
                try {
                    Platform::String ^ name = SanitizeAddress(bindName);
                    if (nullptr == _tcpSocketListener) {
                        _tcpSocketListener = ref new StreamSocketListener();
                        if (nullptr == _tcpSocketListener) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^> ^ handler = ref new TypedEventHandler<StreamSocketListener ^, StreamSocketListenerConnectionReceivedEventArgs ^>(
                            [ = ] (StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args) {
                                TCPSocketConnectionReceived(sender, args);
                            });
                        if (nullptr == handler) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        _tcpSocketListener->ConnectionReceived::add(handler);
                    }
                    if (nullptr != name) {
                        Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(name);
                        if (nullptr == hostname) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        IAsyncAction ^ op = _tcpSocketListener->BindEndpointAsync(hostname, localPort != 0 ? localPort.ToString() : "");
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<void> bindTask(op);
                        bindTask.wait();
                    } else {
                        IAsyncAction ^ op = _tcpSocketListener->BindServiceNameAsync(localPort != 0 ? localPort.ToString() : "");
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<void> bindTask(op);
                        bindTask.wait();
                    }
                    _lastBindPort = _wtoi(_tcpSocketListener->Information->LocalPort->Data());
                    SetBindingState(BindingState::Bind);
                    result = ER_OK;
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
        }
        break;

        case SocketType::QCC_SOCK_DGRAM:
        {
            if (nullptr == _udpSocket ||
                _lastBindHostname != bindName ||
                _lastBindPort != localPort) {
                result = (::QStatus)IsValidAddress(bindName);
                if (ER_OK != result) {
                    break;
                }
                _lastBindHostname = bindName;
                _lastBindPort = localPort;
                try{
                    Platform::String ^ name = SanitizeAddress(bindName);
                    if (nullptr == _udpSocket) {
                        _udpSocket = ref new DatagramSocket();
                        if (nullptr == _udpSocket) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        TypedEventHandler<DatagramSocket ^, DatagramSocketMessageReceivedEventArgs ^> ^ handler = ref new TypedEventHandler<DatagramSocket ^, DatagramSocketMessageReceivedEventArgs ^>(
                            [ = ] (DatagramSocket ^ sender, DatagramSocketMessageReceivedEventArgs ^ args) {
                                UDPSocketMessageReceived(sender, args);
                            });
                        if (nullptr == handler) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        _udpSocket->MessageReceived::add(handler);
                    }
                    if (nullptr != name) {
                        Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(name);
                        if (nullptr == hostname) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        IAsyncAction ^ op = _udpSocket->BindEndpointAsync(hostname, localPort != 0 ? localPort.ToString() : "");
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<void> bindTask(op);
                        bindTask.wait();
                    } else {
                        IAsyncAction ^ op = _udpSocket->BindServiceNameAsync(localPort != 0 ? localPort.ToString() : "");
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<void> bindTask(op);
                        bindTask.wait();
                    }
                    _lastBindPort = _wtoi(_udpSocket->Information->LocalPort->Data());
                    SetBindingState(BindingState::Bind);
                    result = ER_OK;
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
        }
        break;

        default:
            break;
        }
        break;
    }
    _mutex.Unlock();
    if (ER_OK != result) {
        _lastBindHostname = nullptr;
        _lastBindPort = 0;
    }
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Listen(int backlog)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        if ((GetBindingState() & BindingState::Bind) == 0) {
            result = ER_FAIL;
            break;
        }
        SetBindingState(BindingState::Listen);
        _backlog = std::min(backlog, MAX_LISTEN_CONNECTIONS);
        _backlog = std::max(_backlog, 1);
        result = ER_OK;
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

void SocketWrapper::UDPSocketMessageReceived(DatagramSocket ^ sender, DatagramSocketMessageReceivedEventArgs ^ e)
{
    ::QStatus result = ER_OK;
    _mutex.Lock();
    while (true) {
        try {
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            DataReader ^ reader = e->GetDataReader();
            if (nullptr == reader) {
                result = ER_FAIL;
                break;
            }
            IBuffer ^ buffer = reader->DetachBuffer();
            if (nullptr == buffer) {
                result = ER_FAIL;
                break;
            }
            reader = DataReader::FromBuffer(buffer);
            if (nullptr == reader) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            reader->InputStreamOptions = InputStreamOptions::Partial;
            Platform::String ^ remoteHostname = e->RemoteAddress->RawName;
            int remoteBindPort = _wtoi(e->RemotePort->Data());
            UDPMessage ^ msg = ref new UDPMessage(sender, reader, remoteHostname, remoteBindPort);
            if (nullptr == msg) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            _udpBacklog.push_back(msg);
            SetEvent(Events::Read);
            _semReceiveDataQueue.Release();
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
}

void SocketWrapper::TCPSocketConnectionReceived(StreamSocketListener ^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args)
{
    _mutex.Lock();
    while (true) {
        try {
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            // this will bubble up any exceptions
            StreamSocket ^ socket = args->Socket;
            if (socket != nullptr && _tcpBacklog.size() < _backlog) {
                _tcpBacklog.push_back(socket);
                SetEvent(Events::Read);
                _semAcceptQueue.Release();
            } else {
                // reject the connection
                delete socket;
                socket = nullptr;
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
}

uint32_t SocketWrapper::Accept(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort, Platform::WriteOnlyArray<SocketWrapper ^> ^ newSocket)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        if (nullptr == remotePort || remotePort->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        if (nullptr == newSocket || newSocket->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        if ((GetBindingState() & (int)BindingState::Listen) == 0) {
            result = ER_FAIL;
            break;
        }
        if (_blocking) {
            StreamSocket ^ s = nullptr;
            while (nullptr == s && (GetBindingState() & BindingState::Exception) == 0) {
                _mutex.Unlock();
                result = (::QStatus)((int)_semAcceptQueue.Wait());
                _mutex.Lock();
                if (ER_OK != result) {
                    break;
                }
                s = _tcpBacklog.front();
                _tcpBacklog.pop_front();
                if (0 == _tcpBacklog.size()) {
                    ClearEvent(Events::Read);
                }
                if ((GetBindingState() & BindingState::Exception) != 0) {
                    result = ER_FAIL;
                    break;
                }
            }
            if (ER_OK != result) {
                break;
            }
            if (nullptr != s) {
                DataReader ^ reader = ref new DataReader(s->InputStream);
                if (nullptr == reader) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                reader->InputStreamOptions = InputStreamOptions::Partial;
                SocketWrapper ^ tempSocket = ref new SocketWrapper();
                if (nullptr == tempSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                result = (::QStatus)tempSocket->Init(s, reader, _socketAddrFamily);
                if (ER_OK != result) {
                    break;
                }
                remoteAddr[0] = s->Information->LocalAddress->RawName;
                remotePort[0] = _wtoi(s->Information->LocalPort->Data());
                newSocket[0] = tempSocket;
                result = ER_OK;
                break;
            }
        }  else {
            if (_tcpBacklog.size() > 0) {
                result = (::QStatus)((int)_semAcceptQueue.Wait());
                if (ER_OK != result) {
                    break;
                }
                StreamSocket ^ s = nullptr;
                s = _tcpBacklog.front();
                _tcpBacklog.pop_front();
                if (0 == _tcpBacklog.size()) {
                    ClearEvent(Events::Read);
                }
                if ((GetBindingState() & BindingState::Exception) != 0) {
                    result = ER_FAIL;
                    break;
                }
                if (nullptr != s) {
                    DataReader ^ reader = ref new DataReader(s->InputStream);
                    if (nullptr == reader) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    reader->InputStreamOptions = InputStreamOptions::Partial;
                    SocketWrapper ^ tempSocket = ref new SocketWrapper();
                    if (nullptr == tempSocket) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    result = (::QStatus)tempSocket->Init(s, reader, _socketAddrFamily);
                    if (ER_OK != result) {
                        break;
                    }
                    tempSocket->SetBindingState(BindingState::Connect);
                    remoteAddr[0] = s->Information->LocalAddress->RawName;
                    remotePort[0] = _wtoi(s->Information->LocalPort->Data());
                    newSocket[0] = tempSocket;
                    result = ER_OK;
                    break;
                }
            }
            result = ER_WOULDBLOCK;
            break;
        }
        break;
    }
    if (ER_OK != result) {
        if (nullptr != remoteAddr) {
            remoteAddr[0] = nullptr;
        }
        if (nullptr != remotePort) {
            remotePort[0] = 0;
        }
        if (nullptr != newSocket) {
            newSocket[0] = nullptr;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SetBlocking(bool blocking)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        _blocking = blocking;
        result = ER_OK;
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::SetNagle(bool useNagle)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        try {
            if (nullptr == _tcpSocket) {
                _tcpSocket = ref new StreamSocket();
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            _tcpSocket->Control->NoDelay = !useNagle;
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult));
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR);
            break;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

void SocketWrapper::TCPSocketConnectCompleted(IAsyncAction ^ target, AsyncStatus status)
{
    ::QStatus result = ER_OK;
    _mutex.Lock();
    while (true) {
        try {
            // Clean up maps
            if (_connectOperationsMap.find(target->Id) != _connectOperationsMap.end()) {
                _connectOperationsMap.erase(target->Id);
            }
            if (_connectTasksMap.find(target->Id) != _connectTasksMap.end()) {
                _connectTasksMap.erase(target->Id);
            }
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            target->GetResults();
            _dataReader = ref new DataReader(_tcpSocket->InputStream);
            if (nullptr == _tcpSocket) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            _dataReader->InputStreamOptions = InputStreamOptions::Partial;
            SetBindingState(BindingState::Connect);
            QueueTraffic();
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
}

uint32_t SocketWrapper::Connect(Platform::String ^ remoteAddr, int remotePort)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        if (_lastConnectHostname != remoteAddr ||
            _lastConnectPort != remotePort) {
            result = (::QStatus)IsValidAddress(remoteAddr);
            if (ER_OK != result) {
                break;
            }
            _lastConnectHostname = remoteAddr;
            _lastConnectPort = remotePort;
        }
        if (_blocking) {
            if (nullptr == _tcpSocket) {
                _tcpSocket = ref new StreamSocket();
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            if (0 == _connectOperationsMap.size()) {
                try {
                    Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                    if (nullptr == hostname) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    Windows::Foundation::IAsyncAction ^ op = _tcpSocket->ConnectAsync(hostname, remotePort.ToString(), _ssl ? SocketProtectionLevel::Ssl : SocketProtectionLevel::PlainSocket);
                    if (nullptr == op) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    concurrency::task<void> connectTask(op);
                    _connectOperationsMap[op->Id] = op;
                    _connectTasksMap[op->Id] = connectTask.then([ = ] () {
                                                                    TCPSocketConnectCompleted(op, op->Status);
                                                                });
                    concurrency::task<void> connectTask2 = _connectTasksMap[op->Id];
                    _mutex.Unlock();
                    try {
                        connectTask2.wait();
                        _mutex.Lock();
                        result = ER_OK;
                        break;
                    } catch (Platform::COMException ^ ex) {
                        _mutex.Lock();
                        SetLastError(COMExceptionToQStatus(ex->HResult));
                        break;
                    } catch (...) {
                        _mutex.Lock();
                        SetLastError(ER_OS_ERROR);
                        break;
                    }
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
        } else {
            if ((GetBindingState() & BindingState::Connect) != 0) {
                result = ER_OK;
                break;
            }
            if (nullptr == _tcpSocket) {
                _tcpSocket = ref new StreamSocket();
                if (nullptr == _tcpSocket) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            if (0 == _connectOperationsMap.size()) {
                try{
                    Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                    if (nullptr == hostname) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    Windows::Foundation::IAsyncAction ^ op = _tcpSocket->ConnectAsync(hostname, remotePort.ToString(), _ssl ? SocketProtectionLevel::Ssl : SocketProtectionLevel::PlainSocket);
                    if (nullptr == op) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    concurrency::task<void> connectTask(op);
                    _connectOperationsMap[op->Id] = op;
                    _connectTasksMap[op->Id] = connectTask.then([ = ] () {
                                                                    TCPSocketConnectCompleted(op, op->Status);
                                                                });
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult));
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR);
                    break;
                }
            }
            result = ER_WOULDBLOCK;
            break;
        }
        break;
    }
    _mutex.Unlock();
    if (ER_OK != result) {
        _lastConnectHostname = nullptr;
        _lastConnectPort = 0;
    }
    SetLastError(result);
    return result;
}

void SocketWrapper::TCPSocketSendComplete(IAsyncOperationWithProgress<uint32, uint32> ^ target, AsyncStatus status)
{
    _mutex.Lock();
    while (true) {
        try {
            // Clean up maps
            if (_sendOperationsMap.find(target->Id) != _sendOperationsMap.end()) {
                _sendOperationsMap.erase(target->Id);
            }
            if (_sendTasksMap.find(target->Id) != _sendTasksMap.end()) {
                _sendTasksMap.erase(target->Id);
            }
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            int operationCount = _sendOperationsMap.size();
            if (0 == target->GetResults()) {
                // This is a funny way that the tcp stream indicates the other end has closed
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                SetEvent(Events::Exception);
            } else if (operationCount == 0) {
                SetEvent(Events::Write);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
}

void SocketWrapper::UDPSocketSendComplete(IAsyncOperationWithProgress<uint32, uint32> ^ target, AsyncStatus status)
{
    _mutex.Lock();
    while (true) {
        try {
            // Clean up maps
            if (_sendOperationsMap.find(target->Id) != _sendOperationsMap.end()) {
                _sendOperationsMap.erase(target->Id);
            }
            if (_sendTasksMap.find(target->Id) != _sendTasksMap.end()) {
                _sendTasksMap.erase(target->Id);
            }
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            // Clean up maps
            int operationCount = _sendOperationsMap.size();
            target->GetResults();
            if (operationCount == 0) {
                SetEvent(Events::Write);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
}

uint32_t SocketWrapper::SendTo(Platform::String ^ remoteAddr, int remotePort,
                               const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    DatagramSocket ^ sender = nullptr;
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == sent || sent->Length != 1) {
            result = ER_BAD_ARG_5;
            break;
        }
        result = (::QStatus)IsValidAddress(remoteAddr);
        if (ER_OK != result) {
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            result = (::QStatus)Send(buf, len, sent);
            break;

        case SocketType::QCC_SOCK_DGRAM:
        {
            DataWriter ^ dw = ref new DataWriter();
            if (nullptr == dw) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            dw->WriteBytes(buf);
            IBuffer ^ buffer = dw->DetachBuffer();
            if (nullptr == buffer) {
                result = ER_FAIL;
                break;
            }
            if (_udpSocket != nullptr) {
                sender = _udpSocket;
            } else {
                sender = ref new DatagramSocket();
                if (nullptr == sender) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
            }
            IOutputStream ^ writeStream = nullptr;
            try {
                // This is all internal setup work, it doesn't block like a tcp connect
                Windows::Networking::HostName ^ hostname = ref new Windows::Networking::HostName(remoteAddr);
                if (nullptr == hostname) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                Platform::String ^ strRemotePort = remotePort.ToString();
                if (nullptr == strRemotePort) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                IAsyncOperation<IOutputStream ^> ^ getStreamOp = sender->GetOutputStreamAsync(hostname, strRemotePort);
                if (nullptr == getStreamOp) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                concurrency::task<IOutputStream ^> getStreamTask(getStreamOp);
                getStreamTask.wait();
                writeStream = getStreamTask.get();
                if (nullptr == writeStream) {
                    result = ER_OS_ERROR;
                    break;
                }
            } catch (Platform::COMException ^ ex) {
                SetLastError(COMExceptionToQStatus(ex->HResult), true);
                SetEvent(Events::Exception);
                break;
            } catch (...) {
                SetLastError(ER_OS_ERROR, true);
                SetEvent(Events::Exception);
                break;
            }
            if (_blocking) {
                try {
                    IAsyncOperationWithProgress<uint32, uint32>  ^ op = writeStream->WriteAsync(buffer);
                    if (nullptr == op) {
                        result = ER_OUT_OF_MEMORY;
                        break;
                    }
                    concurrency::task<uint32> sendTask(op);
                    _sendOperationsMap[op->Id] = op;
                    ClearEvent(Events::Write);
                    _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                              UDPSocketSendComplete(op, op->Status);
                                                          });
                    concurrency::task<void> sendTask2 = _sendTasksMap[op->Id];
                    _mutex.Unlock();
                    try {
                        sendTask2.wait();
                        sent[0] = sendTask.get();
                        _mutex.Lock();
                        result = ER_OK;
                        break;
                    } catch (Platform::COMException ^ ex) {
                        _mutex.Lock();
                        SetLastError(COMExceptionToQStatus(ex->HResult), true);
                        SetEvent(Events::Exception);
                        break;
                    } catch (...) {
                        _mutex.Lock();
                        SetLastError(ER_OS_ERROR, true);
                        SetEvent(Events::Exception);
                        break;
                    }
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult), true);
                    SetEvent(Events::Exception);
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR, true);
                    SetEvent(Events::Exception);
                    break;
                }
                break;
            } else {
                try{
                    if (_sendOperationsMap.size() == 0) {
                        IAsyncOperationWithProgress<uint32, uint32> ^ op = writeStream->WriteAsync(buffer);
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<uint32> sendTask(op);
                        _sendOperationsMap[op->Id] = op;
                        ClearEvent(Events::Write);
                        _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                  UDPSocketSendComplete(op, op->Status);
                                                              });
                        // not blocking, wait for data to say it completed
                        sent[0] = len;
                        result = ER_OK;
                        break;
                    } else {
                        sent[0] = 0;
                        result = ER_WOULDBLOCK;
                        break;
                    }
                    break;
                } catch (Platform::COMException ^ ex) {
                    SetLastError(COMExceptionToQStatus(ex->HResult), true);
                    SetEvent(Events::Exception);
                    break;
                } catch (...) {
                    SetLastError(ER_OS_ERROR, true);
                    SetEvent(Events::Exception);
                    break;
                }
            }
        }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        if (nullptr != sent) {
            sent[0] = 0;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

void SocketWrapper::ConsumeReaderBytes(Platform::WriteOnlyArray<uint8> ^ buf, uint32 len, Platform::WriteOnlyArray<int> ^ received)
{
    // They really need to fix this (explodes if buffer could receive more than what's available)
    uint32 readCount = std::min(_dataReader->UnconsumedBufferLength, len);
    received[0] = readCount;
    if (readCount > 0) {
        if (readCount == len) {
            _dataReader->ReadBytes(buf);
        } else {
            Platform::Array<uint8> ^ tempBuffer = ref new Platform::Array<uint8>(readCount);
            _dataReader->ReadBytes(tempBuffer);
            memcpy(buf->Data, tempBuffer->Data, readCount);
        }
    }
    // Check if all bytes have been received
    if (_dataReader->UnconsumedBufferLength == 0) {
        if (_socketType == SocketType::QCC_SOCK_STREAM) {
            // tcp
            ClearEvent(Events::Read);
        } else {
            // udp
            if (_udpBacklog.size() == 0) {
                ClearEvent(Events::Read);
            }
        }
    }
}

uint32_t SocketWrapper::RecvFrom(Platform::WriteOnlyArray<Platform::String ^> ^ remoteAddr, Platform::WriteOnlyArray<int> ^ remotePort,
                                 Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == remoteAddr || remoteAddr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        if (nullptr == remotePort || remotePort->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        if (nullptr == buf || buf->Length < 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        if (nullptr == received || received->Length != 1) {
            result = ER_BAD_ARG_5;
            break;
        }
        if ((GetBindingState() & (int)BindingState::Bind) == 0) {
            result = ER_FAIL;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
            result = (::QStatus)Recv(buf, len, received);
            remoteAddr[0] = _lastConnectHostname;
            remotePort[0] = _lastConnectPort;
            break;

        case SocketType::QCC_SOCK_DGRAM:
        {
            if (nullptr != _dataReader && _dataReader->UnconsumedBufferLength > 0) {
                ConsumeReaderBytes(buf, len, received);
                result = ER_OK;
                break;
            } else if (nullptr == _dataReader || _dataReader->UnconsumedBufferLength == 0) {
                if (_blocking) {
                    bool haveData = false;
                    UDPMessage ^ m = nullptr;
                    while (!haveData) {
                        _mutex.Unlock();
                        result = (::QStatus)((int)_semReceiveDataQueue.Wait());
                        _mutex.Lock();
                        if (ER_OK != result) {
                            break;
                        }
                        m = _udpBacklog.front();
                        _udpBacklog.pop_front();
                        if (nullptr != m) {
                            haveData = true;
                        }
                        if ((GetBindingState() & (int)BindingState::Exception) != 0) {
                            result = ER_FAIL;
                            break;
                        }
                    }
                    if (ER_OK != result) {
                        break;
                    }
                    _dataReader = m->Reader;
                    ConsumeReaderBytes(buf, len, received);
                    remoteAddr[0] = m->RemoteHostname;
                    remotePort[0] = m->RemotePort;
                    result = ER_OK;
                    break;
                } else {
                    // See if there's a message and buffer immediately available to consume
                    if (_udpBacklog.size() > 0) {
                        // update the resource count
                        result = (::QStatus)((int)_semReceiveDataQueue.Wait());
                        if (ER_OK != result) {
                            break;
                        }
                        UDPMessage ^ m;
                        m = _udpBacklog.front();
                        _udpBacklog.pop_front();
                        if (nullptr != m) {
                            _dataReader = m->Reader;
                            ConsumeReaderBytes(buf, len, received);
                            remoteAddr[0] = m->RemoteHostname;
                            remotePort[0] = m->RemotePort;
                            result = ER_OK;
                            break;
                        }
                    }
                    // not blocking, no data
                    received[0] = 0;
                    result = ER_WOULDBLOCK;
                    break;
                }
            }
        }
        break;

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        if (nullptr != received) {
            received[0] = 0;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Send(const Platform::Array<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ sent)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == sent || sent->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
        {
            if ((GetBindingState() & BindingState::Connect) != 0) {
                DataWriter ^ dw = ref new DataWriter();
                if (nullptr == dw) {
                    result = ER_OUT_OF_MEMORY;
                    break;
                }
                dw->WriteBytes(buf);
                IBuffer ^ buffer = dw->DetachBuffer();
                if (nullptr == buffer) {
                    result = ER_FAIL;
                    break;
                }
                if (_blocking) {
                    try {
                        IAsyncOperationWithProgress<uint32, uint32>  ^ op = _tcpSocket->OutputStream->WriteAsync(buffer);
                        if (nullptr == op) {
                            result = ER_OUT_OF_MEMORY;
                            break;
                        }
                        concurrency::task<uint32> sendTask(op);
                        _sendOperationsMap[op->Id] = op;
                        ClearEvent(Events::Write);
                        _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                  TCPSocketSendComplete(op, op->Status);
                                                              });
                        concurrency::task<void> sendTask2 = _sendTasksMap[op->Id];
                        _mutex.Unlock();
                        try {
                            sendTask2.wait();
                            sent[0] = sendTask.get();
                            _mutex.Lock();
                            result = ER_OK;
                            break;
                        } catch (Platform::COMException ^ ex) {
                            _mutex.Lock();
                            SetLastError(COMExceptionToQStatus(ex->HResult), true);
                            SetEvent(Events::Exception);
                            break;
                        } catch (...) {
                            _mutex.Lock();
                            SetLastError(ER_OS_ERROR, true);
                            SetEvent(Events::Exception);
                            break;
                        }
                        break;
                    } catch (Platform::COMException ^ ex) {
                        SetLastError(COMExceptionToQStatus(ex->HResult), true);
                        SetEvent(Events::Exception);
                        break;
                    } catch (...) {
                        SetLastError(ER_OS_ERROR, true);
                        SetEvent(Events::Exception);
                        break;
                    }
                } else {
                    try{
                        if (_sendOperationsMap.size() == 0) {
                            IAsyncOperationWithProgress<uint32, uint32> ^ op = _tcpSocket->OutputStream->WriteAsync(buffer);
                            if (nullptr == op) {
                                result = ER_OUT_OF_MEMORY;
                                break;
                            }
                            concurrency::task<uint32> sendTask(op);
                            _sendOperationsMap[op->Id] = op;
                            ClearEvent(Events::Write);
                            _sendTasksMap[op->Id] = sendTask.then([ = ] (uint32 progress) {
                                                                      TCPSocketSendComplete(op, op->Status);
                                                                  });
                            // not blocking, wait for data to say it completed
                            sent[0] = len;
                            result = ER_OK;
                            break;
                        } else {
                            sent[0] = 0;
                            result = ER_WOULDBLOCK;
                            break;
                        }
                        break;
                    } catch (Platform::COMException ^ ex) {
                        SetLastError(COMExceptionToQStatus(ex->HResult), true);
                        SetEvent(Events::Exception);
                        break;
                    } catch (...) {
                        SetLastError(ER_OS_ERROR, true);
                        SetEvent(Events::Exception);
                        break;
                    }
                }
            } else {
                // Not connected
                result = ER_FAIL;
                break;
            }
            break;
        }

        case SocketType::QCC_SOCK_DGRAM:
        {
            if ((GetBindingState() & BindingState::Bind) != 0) {
                result = (::QStatus)SendTo(_lastBindHostname, _lastBindPort, buf, len, sent);
                break;
            }
            // Must have bind
            result = ER_FAIL;
            break;
        }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        if (nullptr != sent) {
            sent[0] = 0;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

void SocketWrapper::TCPStreamLoadComplete(IAsyncOperation<uint32> ^ target, AsyncStatus status)
{
    _mutex.Lock();
    while (true) {
        try {
            // Clean up maps
            if (_receiveOperationsMap.find(target->Id) != _receiveOperationsMap.end()) {
                _receiveOperationsMap.erase(target->Id);
            }
            if (_receiveTasksMap.find(target->Id) != _receiveTasksMap.end()) {
                _receiveTasksMap.erase(target->Id);
            }
            if ((GetBindingState() & BindingState::Exception) != 0) {
                // Drain events if in a tear-down only state
                break;
            }
            if (0 == target->GetResults()) {
                // This is a funny way that the tcp stream indicates the other end has closed
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                SetEvent(Events::Exception);
            } else {
                SetEvent(Events::Read);
            }
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
}

uint32_t SocketWrapper::QueueTraffic()
{
    ::QStatus result = ER_FAIL;
    while (true) {
        // (tcp) If there's no data waiting, and no pending requests to get data, request data
        // (udp) This is already handled with the udp message receiver
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Drain events if in a tear-down only state
            break;
        }
        if (_socketType == SocketType::QCC_SOCK_STREAM &&
            (GetBindingState() & BindingState::Connect) != 0 &&
            _dataReader->UnconsumedBufferLength == 0 &&
            0 == _receiveOperationsMap.size()) {
            // Start a load operation
            Windows::Storage::Streams::DataReaderLoadOperation ^ loadOp = _dataReader->LoadAsync(DEFAULT_READ_SIZE_BYTES);
            if (nullptr == loadOp) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            concurrency::task<uint32> rTask(loadOp);
            _receiveOperationsMap[loadOp->Id] = loadOp;
            _receiveTasksMap[loadOp->Id] = rTask.then([ = ] (uint32 progress) {
                                                          TCPStreamLoadComplete(loadOp, loadOp->Status);
                                                      });
            result = ER_OK;
            break;
        } else {
            result = ER_OK;
            break;
        }
        break;
    }
    if ((GetBindingState() & (int)BindingState::Exception) != 0) {
        result = ER_FAIL;
    }
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Recv(Platform::WriteOnlyArray<uint8> ^ buf, int len, Platform::WriteOnlyArray<int> ^ received)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if (nullptr == buf || buf->Length < 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        if (nullptr == received || received->Length != 1) {
            result = ER_BAD_ARG_3;
            break;
        }
        switch (_socketType) {
        case SocketType::QCC_SOCK_STREAM:
        {
            if ((GetBindingState() & BindingState::Connect) != 0) {
                if (_dataReader->UnconsumedBufferLength == 0) {
                    if (_blocking) {
                        bool haveData = false;
                        while (!haveData) {
                            result = (::QStatus)QueueTraffic();
                            if (ER_OK != result) {
                                break;
                            }
                            concurrency::task<void> rTask = _receiveTasksMap.begin()->second;
                            uint32 id = _receiveOperationsMap.begin()->first;
                            _mutex.Unlock();
                            result = ER_FAIL;
                            try {
                                rTask.wait();
                                _mutex.Lock();
                                if (_dataReader->UnconsumedBufferLength != 0) {
                                    haveData = true;
                                    result = ER_OK;
                                }
                                if ((GetBindingState() & (int)BindingState::Exception) != 0) {
                                    result = ER_FAIL;
                                    break;
                                }
                            } catch (Platform::COMException ^ ex) {
                                _mutex.Lock();
                                SetLastError(COMExceptionToQStatus(ex->HResult), true);
                                SetEvent(Events::Exception);
                                break;
                            } catch (...) {
                                _mutex.Lock();
                                SetLastError(ER_OS_ERROR, true);
                                SetEvent(Events::Exception);
                                break;
                            }
                        }
                        if (ER_OK != result) {
                            break;
                        }
                        ConsumeReaderBytes(buf, len, received);
                        // If we just flushed the reader, try and queue
                        if (_dataReader->UnconsumedBufferLength == 0) {
                            result = (::QStatus)QueueTraffic();
                        } else {
                            result = ER_OK;
                        }
                        break;
                    } else {
                        result = (::QStatus)QueueTraffic();
                        if (ER_OK != result) {
                            break;
                        }
                        received[0] = 0;
                        result = ER_WOULDBLOCK;
                        break;
                    }
                } else {
                    // Buffered
                    ConsumeReaderBytes(buf, len, received);
                    // If we just flushed the reader, try and queue
                    if (_dataReader->UnconsumedBufferLength == 0) {
                        result = (::QStatus)QueueTraffic();
                    } else {
                        result = ER_OK;
                    }
                    break;
                }
            }
            // Not connected
            result = ER_FAIL;
            break;
        }

        case SocketType::QCC_SOCK_DGRAM:
        {
            Platform::Array<Platform::String ^> ^ remoteAddr = ref new Platform::Array<Platform::String ^>(1);
            if (nullptr == remoteAddr) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            Platform::Array<int> ^ remotePort = ref new Platform::Array<int>(1);
            if (nullptr == remotePort) {
                result = ER_OUT_OF_MEMORY;
                break;
            }
            result = (::QStatus)RecvFrom(remoteAddr, remotePort, buf, len, received);
            break;
        }

        default:
            break;
        }
        break;
    }
    if (ER_OK != result &&
        ER_WOULDBLOCK != result) {
        if (nullptr != received) {
            received[0] = 0;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::GetLocalAddress(Platform::WriteOnlyArray<Platform::String ^> ^ addr, Platform::WriteOnlyArray<int> ^ port)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if ((GetBindingState() & BindingState::Bind) == 0 && (GetBindingState() & BindingState::Connect) == 0) {
            result = ER_FAIL;
            break;
        }
        if (nullptr == addr || addr->Length != 1) {
            result = ER_BAD_ARG_1;
            break;
        }
        if (nullptr == port || port->Length != 1) {
            result = ER_BAD_ARG_2;
            break;
        }
        if ((GetBindingState() & BindingState::Bind) == 0  &&
            (GetBindingState() & BindingState::Connect) != 0 &&
            _socketType == SocketType::QCC_SOCK_STREAM) {
            addr[0] = _lastConnectHostname;
            port[0] = _lastConnectPort;
        } else {
            addr[0] = _lastBindHostname;
            port[0] = _lastBindPort;
        }
        result = ER_OK;
        break;
    }
    if (ER_OK != result) {
        if (nullptr != addr) {
            addr[0] = nullptr;
        }
        if (nullptr != port) {
            port[0] = 0;
        }
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Close()
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        if (0 == qcc::winrt::SocketsWrapper::DecrementFDMap(this)) {
            try {
                SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
                Cleanup();
                result = ER_OK;
                break;
            } catch (Platform::COMException ^ ex) {
                SetLastError(COMExceptionToQStatus(ex->HResult), true);
                SetEvent(Events::Exception);
                break;
            } catch (...) {
                SetLastError(ER_OS_ERROR, true);
                SetEvent(Events::Exception);
                break;
            }
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::Shutdown()
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        try {
            SetLastError(ER_SOCK_OTHER_END_CLOSED, true);
            Cleanup();
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult), true);
            SetEvent(Events::Exception);
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR, true);
            SetEvent(Events::Exception);
            break;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

uint32_t SocketWrapper::JoinMulticastGroup(Platform::String ^ host)
{
    ::QStatus result = ER_FAIL;
    _mutex.Lock();
    while (true) {
        if (!_initialized) {
            result = ER_INIT_FAILED;
            break;
        }
        result = (::QStatus)IsValidAddress(host);
        if (ER_OK != result) {
            break;
        }
        if ((GetBindingState() & BindingState::Exception) != 0) {
            // Fail operations on sockets in an exception state
            result = (::QStatus)LastError;
            break;
        }
        if ((GetBindingState() & BindingState::Bind) == 0) {
            // Must not be bound
            result = ER_FAIL;
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_DGRAM) {
            // Must be a UDP socket
            result = ER_FAIL;
            break;
        }
        Windows::Networking::HostName ^ hostName = ref new Windows::Networking::HostName(host);
        if (nullptr == hostName) {
            result = ER_OUT_OF_MEMORY;
            break;
        }
        try {
            _udpSocket->JoinMulticastGroup(hostName);
            result = ER_OK;
            break;
        } catch (Platform::COMException ^ ex) {
            SetLastError(COMExceptionToQStatus(ex->HResult));
            break;
        } catch (...) {
            SetLastError(ER_OS_ERROR);
            break;
        }
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
    return result;
}

void SocketWrapper::Ssl::set(bool ssl)
{
    ::QStatus result = ER_OK;
    _mutex.Lock();
    while (true) {
        if ((GetBindingState() & BindingState::Exception) != 0) {
            break;
        }
        if (_socketType != SocketType::QCC_SOCK_STREAM) {
            result = ER_FAIL;
            break;
        }
        if ((GetBindingState() & BindingState::Connect) != 0) {
            // Must not be connected
            result = ER_FAIL;
            break;
        }
        _ssl = ssl;
        break;
    }
    _mutex.Unlock();
    SetLastError(result);
}

bool SocketWrapper::Ssl::get()
{
    _mutex.Lock();
    bool ssl = _ssl;
    _mutex.Unlock();
    return ssl;
}

void SocketWrapper::SetLastError(uint32_t status, bool isFinal)
{
    _mutex.Lock();
    if (isFinal && (GetBindingState() & BindingState::Exception) == 0) {
        SetBindingState(BindingState::Exception);
        LastError = status;
    } else if ((GetBindingState() & BindingState::Exception) == 0) {
        LastError = status;
    }
    _mutex.Unlock();
}

void SocketWrapper::SetEventMask(int mask)
{
    _mutex.Lock();
    int previousMask = _eventMask;
    _eventMask = mask;
    int currentMask = _eventMask;
    int currentEvents = _events;
    if ((previousMask & currentEvents) !=
        (currentMask & currentEvents)) {
        if (_initialized) {
            _socketEvents.QueueSocketEventsChanged(this, currentEvents & currentMask);
        }
    }
    _mutex.Unlock();
}

int SocketWrapper::GetEvents()
{
    _mutex.Lock();
    int events = _events & _eventMask;
    _mutex.Unlock();
    return events;
}

void SocketWrapper::ExecuteSocketEventsChanged(int flags)
{
    _mutex.Lock();
    if (_initialized) {
        SocketEventsChanged(this, flags);
    }
    _mutex.Unlock();
}

void SocketWrapper::SetEvent(Events e)
{
    _mutex.Lock();
    int previousEvents = _events;
    _events |= (int)e;
    int currentMask = _eventMask;
    int currentEvents = _events;
    if ((currentMask & previousEvents) !=
        (currentMask & currentEvents)) {
        if (_initialized) {
            _socketEvents.QueueSocketEventsChanged(this, currentEvents & currentMask);
        }
    }
    _mutex.Unlock();
}

void SocketWrapper::ClearEvent(Events e)
{
    _mutex.Lock();
    int previousEvents = _events;
    _events &= ~(int)e;
    int currentMask = _eventMask;
    int currentEvents = _events;
    _mutex.Unlock();
}

void SocketWrapper::SetBindingState(BindingState state)
{
    _mutex.Lock();
    _bindingState |= state;
    _mutex.Unlock();
}

int SocketWrapper::GetBindingState()
{
    _mutex.Lock();
    int state = _bindingState;
    _mutex.Unlock();
    return state;
}

void SocketWrapper::ClearBindingState(BindingState state)
{
    _mutex.Lock();
    _bindingState &= ~state;
    _mutex.Unlock();
}

uint32_t SocketWrapper::COMExceptionToQStatus(uint32_t hresult)
{
    uint32_t status = ER_OS_ERROR;
    if (HRESULT_FROM_WIN32(WSAECONNREFUSED) == hresult) {
        status = ER_CONN_REFUSED;
    } else if (HRESULT_FROM_WIN32(WSAECONNRESET) == hresult) {
        status = ER_SOCK_OTHER_END_CLOSED;
    } else if (HRESULT_FROM_WIN32(WSAETIMEDOUT) == hresult) {
        status = ER_TIMEOUT;
    } else {
        status = ER_OS_ERROR;
    }
    return status;
}

}
}
