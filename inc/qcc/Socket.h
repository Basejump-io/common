/**
 * @file
 *
 * Define the abstracted socket interface.
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
#ifndef _QCC_SOCKET_H
#define _QCC_SOCKET_H

#include <qcc/platform.h>

#include <qcc/IPAddress.h>
#include <qcc/ScatterGatherList.h>
#include <qcc/SocketTypes.h>
#include <qcc/SocketWrapper.h>

#include <Status.h>

namespace qcc {

/**
 * Return the error that was set as a result of the last failing (sysem) operation.
 *
 * Many operating systems or system libraries may provide access to a generic
 * error number via a variable, macro or function.  This function provides
 * access to the OS-specific errors in a consistent way; but ultimately, the
 * error number recovered may be system and location specific.
 *
 * @return  The last error set by the underlying system..
 *
 * @see GetLastErrorString()
 *
 * @warning This function returns the last error encountered by the underlying
 * system, not necessarily the last error encountered by this library.
 *
 * @warning This error may valid only after a function known to set the error has
 * actually encountered an error.
 */
uint32_t GetLastError();

/**
 * Map the error number last set by the underlying system to an OS- and
 * locale-dependent error message String.
 *
 * @return A String containing an error message corresponding to the last error
 * number set by the underlying system.
 *
 * @see GetLastError()
 */
qcc::String GetLastErrorString();

/**
 * The maixmum number of files descriptors that can be sent or received by this implementations.
 * See SendWithFds() and RecvWithFds().
 */
static const size_t SOCKET_MAX_FILE_DESCRIPTORS = 16;

/**
 * Open a socket.
 *
 * @param addrFamily    Address family.
 * @param type          Socket type.
 * @param sockfd        OUT: Socket descriptor if successful.
 *
 * @return  Indication of success of failure.
 */
QStatus Socket(AddressFamily addrFamily, SocketType type, SocketFd& sockfd);

/**
 * Connect a socket to a remote host on a specified port
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 *
 * @return  Indication of success of failure.
 */
QStatus Connect(SocketFd sockfd, const IPAddress& remoteAddr, uint16_t remotePort);

/**
 * Connect a local socket
 *
 * @param sockfd        Socket descriptor.
 * @param pathName      Path for the socket
 *
 * @return  Indication of success of failure.
 */
QStatus Connect(SocketFd sockfd, const char* pathName);

/**
 * Bind a socket to an address and port.
 *
 * @param sockfd        Socket descriptor.
 * @param localAddr     IP Address to bind on the local host (maybe 0.0.0.0 or ::).
 * @param localPort     IP Port to bind on the local host.
 *
 * @return  Indication of success of failure.
 */
QStatus Bind(SocketFd sockfd, const IPAddress& localAddr, uint16_t localPort);


/**
 * Bind a socket to a local path name
 *
 * @param sockfd        Socket descriptor.
 * @param pathName      Path for the socket
 *
 * @return  Indication of success of failure.
 */
QStatus Bind(SocketFd sockfd, const char* pathName);


/**
 * Listen for incoming connections on a bound socket.
 *
 * @param sockfd        Socket descriptor.
 * @param backlog       Number of pending connections to queue up.
 *
 * @return  Indication of success of failure.
 */
QStatus Listen(SocketFd sockfd, int backlog);

/**
 * Accept an incoming connection from a remote host.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    OUT: IP Address of remote host.
 * @param remotePort    OUT: IP Port on remote host.
 * @param newSockfd     OUT: New socket descriptor for the connection.
 *
 * @return  Indication of success of failure.
 */
QStatus Accept(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort, SocketFd& newSockfd);


/**
 * Accept an incoming connection from a remote host.
 *
 * @param sockfd        Socket descriptor.
 * @param newSockfd     OUT: New socket descriptor for the connection.
 *
 * @return  Indication of success of failure.
 */
QStatus Accept(SocketFd sockfd, SocketFd& newSockfd);

/**
 * Shutdown a connection.
 *
 * @param sockfd        Socket descriptor.
 *
 * @return  Indication of success of failure.
 */
QStatus Shutdown(SocketFd sockfd);

/**
 * Close a socket descriptor.  This releases the bound port number.
 *
 * @param sockfd        Socket descriptor.
 */
void Close(SocketFd sockfd);

/**
 * Duplicate a socket descriptor.
 *
 * @param sockfd   The socket descriptor to duplicate
 * @param dupSock  [OUT] The duplicated socket descriptor.
 *
 * @return  Indication of success of failure.
 */
QStatus SocketDup(SocketFd sockfd, SocketFd& dupSock);

/**
 * Create a connected pair of (local domain) sockets.
 * @param[out] sockets   Array of two sockects;
 * @return ER_OK if successful.
 */
QStatus SocketPair(SocketFd(&sockets)[2]);

/**
 * Get the local address of the socket.
 *
 * @param sockfd        Socket descriptor.
 * @param addr          IP Address of the local host.
 * @param port          IP Port on local host.
 *
 * @return  Indication of success of failure.
 */
QStatus GetLocalAddress(SocketFd sockfd, IPAddress& addr, uint16_t& port);

/**
 * Send a buffer of data to a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param buf           Pointer to the buffer containing the data to send.
 * @param len           Number of octets in the buffer to be sent.
 * @param sent          OUT: Number of octets sent.
 *
 * @return  Indication of success of failure.
 */
QStatus SendTo(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
               const void* buf, size_t len, size_t& sent);

/**
 * Send a collection of buffers from a scatter-gather list to a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param sg            A scatter-gather list refering to the data to be sent.
 * @param sent          OUT: Number of octets sent.
 *
 * @return  Indication of success of failure.
 */
QStatus SendSG(SocketFd sockfd, const ScatterGatherList& sg, size_t& sent);

/**
 * Send a collection of buffers from a scatter-gather list to a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param sg            A scatter-gather list refering to the data to be sent.
 * @param sent          OUT: Number of octets sent.
 *
 * @return  Indication of success of failure.
 */
QStatus SendToSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t remotePort,
                 const ScatterGatherList& sg, size_t& sent);

/**
 * Receive a buffer of data from a remote host on a socket.
 * This call will block until data is available, the socket is closed.
 *
 * @param sockfd        Socket descriptor.
 * @param buf           Pointer to the buffer where received data will be stored.
 * @param len           Size of the buffer in octets.
 * @param received      OUT: Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus Recv(SocketFd sockfd, void* buf, size_t len, size_t& received);

/**
 * Receive a buffer of data from a remote host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param buf           Pointer to the buffer where received data will be stored.
 * @param len           Size of the buffer in octets.
 * @param received      OUT: Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvFrom(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                 void* buf, size_t len, size_t& received);

/**
 * Receive data into a collection of buffers in a scatter-gather list from a
 * host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param sg            A scatter-gather list refering to buffers where received
 *                      data will be stored.
 * @param received      OUT: Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvSG(SocketFd sockfd, ScatterGatherList& sg, size_t& received);

/**
 * Receive data into a collection of buffers in a scatter-gather list from a
 * host on a socket.
 *
 * @param sockfd        Socket descriptor.
 * @param remoteAddr    IP Address of remote host.
 * @param remotePort    IP Port on remote host.
 * @param sg            A scatter-gather list refering to buffers where received
 *                      data will be stored.
 * @param received      OUT: Number of octets received.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvFromSG(SocketFd sockfd, IPAddress& remoteAddr, uint16_t& remotePort,
                   ScatterGatherList& sg, size_t& received);


/**
 * Receive a buffer of data from a remote host on a socket and any file descriptors accompanying the
 * data.  This call will block until data is available, the socket is closed.
 *
 * @param sockfd     Socket descriptor.
 * @param buf        Pointer to the buffer where received data will be stored.
 * @param len        Size of the buffer in octets.
 * @param received   OUT: Number of octets received.
 * @param fdList     The file descriptors received.
 * @param maxFds     The maximum number of file descriptors (size of fdList)
 * @param recvdFds   The number of file descriptors received.
 *
 * @return  Indication of success of failure.
 */
QStatus RecvWithFds(SocketFd sockfd, void* buf, size_t len, size_t& received, SocketFd* fdList, size_t maxFds, size_t& recvdFds);

/**
 * Send a buffer of data with file descriptors to a socket. Depending on the transport this may may use out-of-band
 * or in-band data or some mix of the two.
 *
 * @param sockfd    Socket descriptor.
 * @param buf       Pointer to the buffer containing the data to send.
 * @param len       Number of octets in the buffer to be sent.
 * @param sent      [OUT] Number of octets sent.
 * @param fdList    Array of file descriptors to send.
 * @param numFds    Number of files descriptors.
   @param pid       Process id required on some platforms.
 *
 * @return  Indication of success of failure.
 */
QStatus SendWithFds(SocketFd sockfd, const void* buf, size_t len, size_t& sent, SocketFd* fdList, size_t numFds, uint32_t pid);

/**
 * Set a socket to blocking or not blocking.
 *
 * @param sockfd    Socket descriptor.
 * @param blocking  If true set it to blocking, otherwise no-blocking.
 */
QStatus SetBlocking(SocketFd sockfd, bool blocking);

/**
 * Set TCP based socket to use or not use Nagle algorithm (TCP_NODELAY)
 *
 * @param sockfd    Socket descriptor.
 * @param useNagle  Set to true to Nagle algorithm. Set to false to disable Nagle.
 */
QStatus SetNagle(SocketFd sockfd, bool useNagle);

/**
 * @brief Allow a service to bind to a TCP endpoint which is in the TIME_WAIT
 * state.
 *
 * Setting this option allows a service to be restarted after a crash (or
 * contrl-C) and then be restarted without having to wait for some possibly
 * significant (on the order of minutes) time.
 *
 * @see SetReusePort()
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param reuse  Set to true to allow address and/or port reuse.
 */
QStatus SetReuseAddress(SocketFd sockfd, bool reuse);

/**
 * @brief Allow multiple services to bind to the same address and port.
 *
 * Setting this option allows a multiple services to bind to the same address
 * and port.  This is typically useful for multicast operations where more than
 * one process might want to listen on the same muticast address and port.  In
 * order to use this function successfully, ALL processes that want to listen on
 * a common port must set this option.
 *
 * @warning This function sets the socket option SO_REUSEPORT when it is
 * available but falls back to SO_REUSEADDR if it is not.  The original socket
 * option was a BSD-ism that was introduced when multicast was added there.  Not
 * all stacks support SO_REUSEPORT, but to accomplish the same thing they will
 * special-case SO_REUSEADDR in the presence of multicast to accomplish the same
 * functionality.  In this case, there is no functional difference between
 * SetReusePort() and SetReuseAddress(), but the two functions remain for
 * clarity of purpose.
 *
 * @see SetReuseAddress()
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param reuse  Set to true to allow address and/or port reuse.
 */
QStatus SetReusePort(SocketFd sockfd, bool reuse);

/**
 * Ask a UDP-based socket to join the specified multicast group.
 *
 * Arrange for the system to perform an IGMP join and enable reception of
 * packets addressed to the provided multicast group.  The details of the
 * particular process used to drive the join depend on the address family of the
 * socket.  Since this call may be made on a bound or unbound socket, and there
 * is no way to discover the address family from an unbound socket, we require
 * that the address family of the socket be provided in this call.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param family The address family used to create the provided sockfd.
 * @param multicastGroup A string containing the desired multicast group in
 *     presentation format.
 * @param interface A string containing the interface name (e.g., "wlan0") on
 *     which to join the group.
 */
QStatus JoinMulticastGroup(SocketFd sockfd, AddressFamily family, String multicastGroup, String interface);

/**
 * Ask a UDP-based socket to join the specified multicast group.
 *
 * Arrange for the system to perform an IGMP leave and disable reception of
 * packets addressed to the provided multicast group.  The details of the
 * particular process used to drive the join depend on the address family of the
 * socket.  Since this call may be made on a bound or unbound socket, and there
 * is no way to discover the address family from an unbound socket, we require
 * that the address family of the socket be provided in this call.
 *
 * @param sockFd The socket file descriptor identifying the resource.
 * @param family The address family used to create the prvided socket.
 * @param multicastGroup A string containing the desired multicast group in
 *     presentation format.
 * @param interface A string containing the interface name (e.g., "wlan0") on
 *     which to join the group.
 */
QStatus LeaveMulticastGroup(SocketFd socketFd, AddressFamily family, String multicastGroup, String iface);

/**
 * Set the outbound interface over which multicast packets are sent using
 * the provided socket.
 *
 * Override the internal OS routing of multicast packets and specify which
 * single interface over which multicast packets sent using this socket will be
 * sent.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param interface A string containing the desired interface (e.g., "wlan0"),
 */
QStatus SetMulticastInterface(SocketFd socketFd, AddressFamily family, qcc::String iface);

/**
 * Set the number of hops over which multicast packets will be routed when
 * sent using the provided socket.
 *
 * @param sockfd The socket file descriptor identifying the resource.
 * @param hops The desired number of hops.
 */
QStatus SetMulticastHops(SocketFd socketFd, AddressFamily family, uint32_t hops);

/**
 * Set the broadcast option on the provided socket.
 *
 * @param sockfd The socket descriptor identifying the resource.
 * @param broadcast  Set to true to enable broadcast on the socket.
 */
QStatus SetBroadcast(SocketFd sockfd, bool broadcast);


} // namespace qcc

#endif // _QCC_SOCKET_H
