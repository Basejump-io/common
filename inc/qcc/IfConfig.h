/**
 * @file
 * Data structure and declaration of function for getting network interface configurations
 */

/******************************************************************************
 * Copyright 2010-2011, Qualcomm Innovation Center, Inc.
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

#ifndef _IFCONFIG_H
#define _IFCONFIG_H

#ifndef __cplusplus
#error Only include IfConfig.h in C++ code.
#endif

#include <vector>
#include <Status.h>
#include <qcc/String.h>
#include <qcc/SocketTypes.h>

namespace qcc {

class IfConfigEntry {
  public:
    qcc::String m_name;     /**< The operating system-assigned name of this interface (e.g. "eth0" or "wlan0"). */
    qcc::String m_addr;     /**< A string representation of the IP address of this interface. */
    uint32_t m_prefixlen;   /**< The network prefix length, in the sense of CIDR, for the IP address of this interface. */
    AddressFamily m_family; /**< The address family of the IP address of this interface (AF_UNSPEC, AF_INET or AF_INET6). */

    static const uint32_t UP = 1;               /**< The interface is running and routes are in place. */
    static const uint32_t BROADCAST = 2;        /**< The interface has a valid broadcast address (can broadcast). */
    static const uint32_t DEBUG = 4;            /**< The underlying interface is in debug mode. */
    static const uint32_t LOOPBACK = 8;         /**< This is a loopback interface. */
    static const uint32_t POINTOPOINT = 16;     /**< This interface runs over a point to point link. */
    static const uint32_t RUNNING = 32;         /**< The hardware is running and can send and receive packets. */
    static const uint32_t NOARP = 64;           /**< There is no Address Resolution Protocol required or running. */
    static const uint32_t PROMISC = 128;        /**< The underlying device is in promiscuous mode. */
    static const uint32_t NOTRAILERS = 256;     /**< Avoid the use of trailers in BSD. */
    static const uint32_t ALLMULTI = 512;       /**< Receive all multicast packets.  Useful for multicast routing. */
    static const uint32_t MASTER = 1024;        /**< Load equalization code flag. */
    static const uint32_t SLAVE = 2048;         /**< Load equalization code flag. */
    static const uint32_t MULTICAST = 4096;     /**< The interface as capable of multicast transmission. */
    static const uint32_t PORTSEL = 8192;       /**< Marks the interface as capable of switching between media types. */
    static const uint32_t AUTOMEDIA = 16384;    /**< The interface is capable of automatically choosing media type. */
    static const uint32_t DYNAMIC = 32768;      /**< This interface has an IP address that can change (currently unused). */

    uint32_t m_flags;   /**< The combined interface flags for this interface. */
    uint32_t m_mtu;     /**< The maximum transmission unit (MTU) for this interface. */
    uint32_t m_index;   /**< The operating system generated interface index for this interface. */
};

/**
 * @brief Get information regarding the network interfaces on the
 * host.
 *
 * In the mobile device environment, it is often the case that network
 * interfaces will come up and go down unpredictably as the underlying Wi-Fi is
 * associated with or disassociated from access points as the device physically
 * moves.
 *
 * Different operating systems return different tidbits of information regarding
 * their network interfaces using sometimes wildly differing mechanisms, and
 * reporting what is conceptually the same information in sometimes wildly
 * differing formats.
 *
 * It is often desired to get information about network interfaces irrespective
 * of whether or not IP addresses are assigned or whether IPv4 or IPv6 addresses
 * may be present.
 *
 * This function provides an OS-indepenent way of reporting network interface
 * information in a relatively abstract way, providing as much or as little
 * information as may be available at any time.
 *
 * @param entries A vector of IfConfigEntry that will be filled out
 *     with information on the found network interfaces.
 *
 * @return Status of the operation.  Returns ER_OK on success.
 *
 * @see IfConfigEntry
 */
QStatus IfConfig(std::vector<IfConfigEntry>& entries);

} // namespace ajn

#endif // _IFCONFIG_H
