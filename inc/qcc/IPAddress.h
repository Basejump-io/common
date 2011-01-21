/**
 * @file
 *
 * This file defines the IP Address abstraction class.
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

#ifndef _QCC_IPADDRESS_H
#define _QCC_IPADDRESS_H


#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <assert.h>
#include <string.h>

#include <qcc/SocketTypes.h>

#include <Status.h>

/** @internal */
#define QCC_MODULE "NETWORK"

static const uint32_t MAX_IPV4_MTU = 576;    ///< Max UDP payload size for IPv4 defined in RFC 5389.
static const uint32_t MAX_IPV6_MTU = 1280;   ///< Max UDP payload size for IPv6 defined in RFC 5389.


namespace qcc {

/**
 * IP Address class for handling IPv4 and IPv6 addresses uniformly.  Even
 * supports automtic mapping of IPv4 addresses on to IPv6 address spaces.
 */
class IPAddress {
  public:
    static const size_t IPv4_SIZE = 4;  ///< Size of IPv4 address.
    static const size_t IPv6_SIZE = 16; ///< Size of IPv6 address.

  private:
    uint8_t addr[IPv6_SIZE];    ///< Storage for IP address.
    uint16_t addrSize;          ///< IP address size (indirectly indicates IPv4 vs. IPv6).

  public:
    /**
     * Default constructor initializes an invalid IP address.  Generally, this
     * should not be used but is still made available if needed.
     */
    IPAddress(void) : addrSize(0) { ::memset(addr, 0, sizeof(addr)); }

    /**
     * Consruct IPAddress based on a string containing an IPv4 or IPv6 address.
     *
     * @param addrString    Reference to the string with the IP address.
     */
    IPAddress(const qcc::String& addrString);

    /**
     * Set or change the address that an existing IPAddress refers to.
     * Using this method instead of using the constructor allows
     * errors to be returned.
     *
     * @param addrString  IP address (V4 or V6) or hostname.
     * @return ER_OK if successful.
     */
    QStatus SetAddress(const qcc::String& addrString);

    /**
     * Consruct IPAddress based on a buffer containing an IPv4 or IPv6 address.
     *
     * @param addrBuf       Pointer to the buffer with the IP address.
     * @param addrBufSize   Size of the address buffer.
     */
    IPAddress(const uint8_t* addrBuf, size_t addrBufSize)
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

    /**
     * Construct and IPv4 address from a 32-bit integer.
     *
     * @param ipv4Addr  32-bit integer representation of the IP address.
     */
    IPAddress(uint32_t ipv4Addr)
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


    /**
     * Compare equality between 2 IPAddress's.
     *
     * @param other     The other IPAddress to compare against.
     *
     * @return  'true' if they are the same, 'false' otherwise.
     */
    bool operator==(const IPAddress& other) const
    {
        return ((Size() == other.Size()) &&
                (memcmp(GetIPReference(), other.GetIPReference(), Size()) == 0));
    }


    /**
     * Compare inequality between 2 IPAddress's.
     *
     * @param other     The other IPAddress to compare against.
     *
     * @return  'true' if they are not the same, 'false' otherwise.
     */
    bool operator!=(const IPAddress& other) const { return !(*this == other); }


    /**
     * Get the size of the IP address.
     *
     * @return  The size of the IP address.
     */
    size_t Size(void) const { return addrSize; }

    /**
     * Test if IP address is an IPv4 address.
     *
     * @return  "true" if IP address in an IPv4 address.
     */
    bool IsIPv4(void) const { return addrSize == IPv4_SIZE; }

    /**
     * Test if IP address is an IPv6 address.
     *
     * @return  "true" if IP address in an IPv6 address.
     */
    bool IsIPv6(void) const { return addrSize == IPv6_SIZE; }

    /**
     * Convert the IP address into a human readable form in a string.  IPv4
     * addresses will use the standard "dot-quad" notation (i.e., 127.0.0.1)
     * and IPv6 addresses will use the standard notation as defined in RFC
     * 4291 (i.e., ::1).
     *
     * @return  The string representation of the IP address.
     */
    qcc::String ToString(void) const
    {
        if (addrSize == IPv4_SIZE) {
            return IPv4ToString(&addr[IPv6_SIZE - IPv4_SIZE]);
        } else if (addrSize == IPv6_SIZE) {
            return IPv6ToString(addr);
        } else {
            return qcc::String("<invalid IP address>");
        }
    }

    /**
     * Helper function to convert an IPv4 address in a buffer to a string.
     *
     * @param addrBuf   Buffer at least 4 octets long containing the IPv4 address.
     *
     * @return  The string representation of the IPv4 address.
     */
    static qcc::String IPv4ToString(const uint8_t addrBuf[]);

    /**
     * Helper function to convert an IPv6 address in a buffer to a string.
     *
     * @param addrBuf   Buffer at least 16 octets long containing the IPv6 address.
     *
     * @return  The string representation of the IPv6 address.
     */
    static qcc::String IPv6ToString(const uint8_t addrBuf[]);


    /**
     * Renders the IPv4 address in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IPv4 address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPv4Binary(uint8_t addrBuf[], size_t addrBufSize) const
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

    /**
     * Renders the IPv6 address in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IPv6 address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPv6Binary(uint8_t addrBuf[], size_t addrBufSize) const
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

    /**
     * Renders the IP address (IPv4 or IPv6) in binary format into a buffer.
     *
     * @param addrBuf       An array for storing the IP address.
     * @param addrBufSize   Size of the address buffer.
     *
     * @return  Status indicating success or failure.
     */
    QStatus RenderIPBinary(uint8_t addrBuf[], size_t addrBufSize) const
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


    /**
     * Get a direct reference to the IPv6 address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPv6Reference(void) const
    {
        return addr;
    }

    /**
     * Get a direct reference to the IPv4 address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPv4Reference(void) const
    {
        return &addr[IPv6_SIZE - IPv4_SIZE];
    }

    /**
     * Get a direct reference to the IP address.
     *
     * @return  A constant pointer to the address buffer.
     */
    const uint8_t* GetIPReference(void) const
    {
        return &addr[IPv6_SIZE - addrSize];
    }

    /**
     * Get the IPv4 address as a 32-bit unsigned integer in CPU order.
     *
     * @return  A 32-bit unsigned integer representation of the IPv4 address.
     */
    uint32_t GetIPv4AddressCPUOrder(void) const
    {
        return ((static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 0]) << 24) |
                (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 1]) << 16) |
                (static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 2]) << 8) |
                static_cast<uint32_t>(addr[IPv6_SIZE - IPv4_SIZE + 3]));
    }

    /**
     * Get the IPv4 address as a 32-bit unsigned integer in network order.
     *
     * @return  A 32-bit unsigned integer representation of the IPv4 address.
     */
    uint32_t GetIPv4AddressNetOrder(void) const
    {
        uint32_t addr4;
        memcpy(&addr4, &addr[IPv6_SIZE - IPv4_SIZE], IPv4_SIZE);
        return addr4;
    }

    /**
     * Convert the IP address to an IPv4 address.
     *
     * @attention This is only useful for converting IPv6 addresses that were
     * previously converted from an IPv4 address.
     */
    void ConvertToIPv4(void) { addrSize = IPv4_SIZE; }

    /**
     * Convert the IP address to an IPv6 address.  This results in an
     * IPv4-mapped-on-IPv6 address (e.g., ::ffff:a0a:2020 for 10.10.32.32).
     */
    void ConvertToIPv6(void) { addrSize = IPv6_SIZE; }

    /**
     * Get the address family for the IPAddress.
     *
     * @return  The address family for this address.
     */
    AddressFamily GetAddressFamily(void) const
    {
        return (IsIPv4() ? QCC_AF_INET : QCC_AF_INET6);
    }
};


/**
 * IpEndpoint describes an address/port endpoint for an IP-based connection.
 *
 */
struct IPEndpoint {
    /** Address */
    qcc::IPAddress addr;

    /** Port */
    uint16_t port;

    /**
     * Equality test
     * @param other  Endpoint to compare with.
     * @return true iff other equals this IPAddress.
     */
    bool operator==(const qcc::IPEndpoint& other) const
    {
        return ((addr == other.addr) &&
                (port == other.port));
    }

};

}

#undef QCC_MODULE
#endif
