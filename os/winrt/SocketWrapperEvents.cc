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

#include <qcc/platform.h>
#include <qcc/winrt/SocketWrapper.h>
#include <qcc/winrt/SocketWrapperEvents.h>

using namespace Windows::Foundation;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/** @internal */
#define QCC_MODULE "SOCKETWRAPPEREVENTS"

namespace qcc {
namespace winrt {

SocketWrapperEvents::SocketWrapperEvents()
{
}

void SocketWrapperEvents::QueueSocketEventsChanged(SocketWrapper ^ socket, int flags)
{
    void* sockfd = reinterpret_cast<void*>(socket);
    _coalescedSocketEventMapMutex.Lock();
    if (_coalescedSocketEventMap.find(sockfd) != _coalescedSocketEventMap.end()) {
        // In map, update flags
        int oldFlags = _coalescedSocketEventMap[sockfd];
        _coalescedSocketEventMap[sockfd] = oldFlags | flags;
    } else {
        // Not in map, add socket
        _coalescedSocketEventMap[sockfd] = flags;

        // Async dispatch
        qcc::IncrementAndFetch(&(socket->_callbackCount));
        concurrency::create_async([socket, this](concurrency::cancellation_token ct) {
                                      DispatchSocketEventsChanged(socket);
                                      qcc::DecrementAndFetch(&(socket->_callbackCount));
                                  });
    }
    _coalescedSocketEventMapMutex.Unlock();
}

void SocketWrapperEvents::DispatchSocketEventsChanged(SocketWrapper ^ socket)
{
    void* sockfd = reinterpret_cast<void*>(socket);
    _coalescedSocketEventMapMutex.Lock();
    int flags = _coalescedSocketEventMap[sockfd];
    _coalescedSocketEventMap.erase(sockfd);
    _coalescedSocketEventMapMutex.Unlock();

    try {
        socket->ExecuteSocketEventsChanged(flags);
    } catch (...) {
        // Don't let application errors kill off the event thread
    }
}

}
}
