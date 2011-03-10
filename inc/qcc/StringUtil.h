/**
 * @file
 *
 * This file defines string related utility functions
 */

/******************************************************************************
 *
 *
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

#ifndef _QCC_STRINGUTIL_H
#define _QCC_STRINGUTIL_H

#include <qcc/platform.h>
#include <qcc/String.h>

namespace qcc {

/**
 * Convert byte array to hex representation.
 *
 * @param inBytes    Pointer to byte array.
 * @param len        Number of bytes.
 * @param toLower    TRUE to use a-f, FALSE for A-F
 * @param separator  Separator to use between each byte
 * @return Hex string representation of inBytes.
 */
qcc::String BytesToHexString(const uint8_t* inBytes, size_t len, bool toLower = false, char separator = 0);


/**
 * Convert hex string to a byte array representation.
 *
 * @param hex        String containing hex encoded data
 * @param outBytes   Pointer to byte array.
 * @param len        Size of byte array
 * @param separator  Separator to expect between each byte
 * @return Number of bytes written
 */
size_t HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len, char separator = 0);


/**
 * Convert hex string to a string of bytes
 *
 * @param hex        String containing hex encoded data
 * @param separator  Separator to expect between each byte
 * @return A string containing the converted bytes or an empty string if the conversion failed.
 */
qcc::String HexStringToByteString(const qcc::String& hex, char separator = 0);


/**
 * Generate a random hex string.
 */
qcc::String RandHexString(size_t len, bool toLower = false);


/**
 * Convert uint32_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String U32ToString(uint32_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');

/**
 * Convert int32_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String I32ToString(int32_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert uint64_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String U64ToString(uint64_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert int64_t to a string.
 *
 * @param num     Number to convert.
 * @param base    Base (radix) for output string. (Must be between 1 and 16 inclusive). Defaults to 10.
 * @param width   Minimum amount of space in string the conversion will use.
 * @param fill    Fill character.
 * @return  String representation of num.
 */
qcc::String I64ToString(int64_t num, unsigned int base = 10, size_t width = 1, char fill = ' ');


/**
 * Convert decimal or hex formatted string to a uint32_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
uint32_t StringToU32(const qcc::String& inStr, unsigned int base = 0, uint32_t badValue = 0);


/**
 * Convert decimal or hex formatted string to an int32_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
int32_t StringToI32(const qcc::String& inStr, unsigned int base = 0, int32_t badValue = 0);


/**
 * Convert decimal or hex formatted string to a uint64_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
uint64_t StringToU64(const qcc::String& inStr, unsigned int base = 0, uint64_t badValue = 0);


/**
 * Convert decimal or hex formatted string to an int64_t.
 *
 * @param inStr     String representation of number.
 * @param base      Base (radix) representation of inStr. 0 indicates autodetect according to C nomenclature. Defaults to 0. (Must be between 0 and 16).
 * @param badValue  Value returned if string (up to EOS or first whitespace character) is not parsable as a number.
 */
int64_t StringToI64(const qcc::String& inStr, unsigned int base = 0, int64_t badValue = 0);


/**
 * Convert numeric string to an double.
 *
 * @param inStr     String representation of number.
 */
double StringToDouble(const qcc::String& inStr);


/**
 * Remove leading and trailing whilespace from string.
 *
 * @param inStr  Input string.
 * @return  inStr with leading and trailing whitespace removed.
 */
qcc::String Trim(const qcc::String& inStr);


/**
 * Test whether character is a white space character.
 *
 * @param c            Character to test.
 * @param whiteChars   Optional Null terminated "C" string containing white chars. If not
 *                     specified, " \t\r\n" is used as the set of white space chars.
 * @return true iff c is a white space character.
 */
bool IsWhite(char c, const char* whiteChars = 0);

}   /* End namespace */

#endif
