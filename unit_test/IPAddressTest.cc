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
 ******************************************************************************/
#include <gtest/gtest.h>
#include <qcc/IPAddress.h>

using namespace qcc;

// Note: 'test/IPAddress_test.cc' already exists.
//       However, it pre-dates the use of googletest framework.
//
//       The current file, 'unit_test/IPAddressTest.cc', attempts to replace
//       the above mentioned file at some point.

TEST(IPAddressTest, ipv4_to_string) {
    uint8_t localhost[] = { 127, 0, 0, 1 };
    String expected_string_representation = String("127.0.0.1");
    String actual_string_representation = IPAddress::IPv4ToString(localhost);

    EXPECT_STREQ(expected_string_representation.c_str(),
                 actual_string_representation.c_str()) <<
    "The function IPv4ToString did not return \"" <<
    expected_string_representation.c_str() << "\", when passed "
    "the byte array: {" <<
    (unsigned int) localhost[0] << ", " <<
    (unsigned int) localhost[1] << ", " <<
    (unsigned int) localhost[2] << ", " <<
    (unsigned int) localhost[3] << "}.";
}

TEST(IPAddressTest, string_to_ipv4) {
    String localhost = String("127.0.0.1");

    uint8_t* address_buffer = new uint8_t [IPAddress::IPv4_SIZE];

    QStatus status = IPAddress::StringToIPv4(localhost, address_buffer,
                                             IPAddress::IPv4_SIZE);
    EXPECT_EQ(ER_OK, status) <<
    "The function StringToIPv4 was unable to convert the string \"" <<
    localhost.c_str() << "\" to a byte array.";

    if (ER_OK == status) {
        uint8_t expected_address_buffer[] = { 127, 0, 0, 1 };

        for (size_t i = 0; i < IPAddress::IPv4_SIZE; i++) {
            EXPECT_EQ(expected_address_buffer[i], address_buffer[i]) <<
            "At index " << i << ", the octet value " <<
            (unsigned int) address_buffer[i] << ", of the byte array "
            "converted from string \"" << localhost.c_str() <<
            "\" by the function StringToIPv4 does not match the "
            "expected value " << (unsigned int) expected_address_buffer[i];
        }
    }

    // Clean-up
    delete [] address_buffer;
    address_buffer = NULL;
}

TEST(IPAddressTest, string_to_ipv4_other_bases_viz_octal_hex) {
    // Glass is half-full
    QStatus status = ER_FAIL;

    uint8_t* address_buffer = new uint8_t [IPAddress::IPv4_SIZE];

    String google_public_dns_server_in_decimal = String("8.8.8.8");
    // decimal digit 8 == octal digit 010
    String google_public_dns_server_in_octal = String("010.010.010.010");

    status = IPAddress::StringToIPv4(google_public_dns_server_in_octal,
                                     address_buffer,
                                     IPAddress::IPv4_SIZE);
    EXPECT_EQ(ER_OK, status) <<
    "The function StringToIPv4 was unable to convert the string \"" <<
    google_public_dns_server_in_octal.c_str() << "\" to a byte array.";

    if (ER_OK == status) {
        // Convert the address_buffer back to a string and compare
        String converted_string = IPAddress::IPv4ToString(address_buffer);

        EXPECT_STREQ(google_public_dns_server_in_decimal.c_str(),
                     converted_string.c_str()) <<
        "The ip address string \"" <<
        google_public_dns_server_in_octal.c_str() << "\" (in octal) "
        "was converted to a byte array and re-converted back to a string "
        "(in decimal). The converted string \"" << converted_string.c_str() <<
        "\" isn't matching the expected string \"" <<
        google_public_dns_server_in_decimal.c_str() << "\".";
    }

    String open_dns_server_in_decimal = String("208.67.222.222");
    // decimal digit 208 = 13 * 16 +  0 = 0xD0
    // decimal digit  67 = 04 * 16 +  3 = 0x43
    // decimal digit 222 = 13 * 16 + 14 = 0xDE
    String open_dns_server_in_hex = String("0xD0.0x43.0xDE.0xDE");

    status = IPAddress::StringToIPv4(open_dns_server_in_hex,
                                     address_buffer,
                                     IPAddress::IPv4_SIZE);
    EXPECT_EQ(ER_OK, status) <<
    "The function StringToIPv4 was unable to convert the string \"" <<
    open_dns_server_in_hex.c_str() << "\" to a byte array.";

    if (ER_OK == status) {
        // Convert the address_buffer back to a string and compare
        String converted_string = IPAddress::IPv4ToString(address_buffer);

        EXPECT_STREQ(open_dns_server_in_decimal.c_str(),
                     converted_string.c_str()) <<
        "The ip address string \"" <<
        open_dns_server_in_hex.c_str() << "\" (in hex) was converted to "
        "a byte array and re-converted back to a string (in decimal). "
        "The converted string \"" << converted_string.c_str() <<
        "\" isn't matching the expected string \"" <<
        open_dns_server_in_decimal.c_str() << "\".";
    }

    // Clean-up
    delete [] address_buffer;
    address_buffer = NULL;
}

TEST(IPAddressTest, string_to_ipv4_negative_test_cases) {
    String some_ip_address_string = String("some-string-literal-value");

    uint8_t* address_buffer = new uint8_t [IPAddress::IPv4_SIZE];

    // a bad second argument - NULL buffer pointer
    EXPECT_EQ(ER_BAD_ARG_2,
              IPAddress::StringToIPv4(some_ip_address_string, NULL,
                                      IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained when passed a NULL value "
    "as second parameter.";

    // The googletest macros seem to cause linker errors when asked to print
    // static class member variables viz.
    // qcc::IPAddress::IPv4_SIZE and qcc::IPAddress::IPv6_SIZE
    // Hence, using the following two constant
    // while printing the error message.
    const size_t ipv4_size = IPAddress::IPv4_SIZE;
    const size_t ipv6_size = IPAddress::IPv6_SIZE;

    // a bad third argument - passing 16 octets instead of 4
    EXPECT_EQ(ER_BAD_ARG_3,
              IPAddress::StringToIPv4(some_ip_address_string, address_buffer,
                                      IPAddress::IPv6_SIZE)) <<
    "The function StringToIPv4 should have complained when passed " <<
    ipv6_size << " (an incompatible value), instead of " <<
    ipv4_size << " as third parameter.";

    // missing the first octet
    some_ip_address_string = String(".0.0.1");
    EXPECT_EQ(ER_PARSE_ERROR,
              IPAddress::StringToIPv4(some_ip_address_string, address_buffer,
                                      IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained while parsing "
    "the string \"" << some_ip_address_string.c_str() << "\".";

    // missing the second octet
    some_ip_address_string = String("127..0.1");
    EXPECT_EQ(ER_PARSE_ERROR,
              IPAddress::StringToIPv4(some_ip_address_string,
                                      address_buffer, IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained while parsing "
    "the string \"" << some_ip_address_string.c_str() << "\".";

    // missing the third octet
    some_ip_address_string = String("127.0..1");
    EXPECT_EQ(ER_PARSE_ERROR,
              IPAddress::StringToIPv4(some_ip_address_string,
                                      address_buffer, IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained while parsing "
    "the string \"" << some_ip_address_string.c_str() << "\".";

    // too many octets
    some_ip_address_string = String("127.0.0.0.1");
    EXPECT_EQ(ER_PARSE_ERROR,
              IPAddress::StringToIPv4(some_ip_address_string,
                                      address_buffer, IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained while parsing "
    "the string \"" << some_ip_address_string.c_str() << "\".";

    // reasonable ip-address:port, but incompatible as an ipaddress.
    some_ip_address_string = String("127.0.0.1:443");
    EXPECT_EQ(ER_PARSE_ERROR,
              IPAddress::StringToIPv4(some_ip_address_string,
                                      address_buffer, IPAddress::IPv4_SIZE)) <<
    "The function StringToIPv4 should have complained while parsing "
    "the string \"" << some_ip_address_string.c_str() << "\".";

    // Clean-up
    delete [] address_buffer;
    address_buffer = NULL;
}
