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
#include <qcc/Util.h>
#include <qcc/StringUtil.h>

using namespace qcc;

TEST(StringUtilTest, hex_string_to_byte_array_conversion_off_by_one) {
    // String of odd length - "fee"
    String fee = String("fee");
    // The substring, "fe", of even length
    String substring_of_fee = fee.substr(0, (fee.length() - 1));

    uint8_t* bytes_corresponding_to_string = (uint8_t*) malloc(
        sizeof(uint8_t) * fee.length() / 2);
    uint8_t* bytes_corresponding_to_substring = (uint8_t*) malloc(
        sizeof(uint8_t) * substring_of_fee.length() / 2);

    // Intentionally ignoring the return values,
    // as we are not checking for them in this particular scenario.
    HexStringToBytes(fee, bytes_corresponding_to_string, fee.length() / 2);
    HexStringToBytes(substring_of_fee, bytes_corresponding_to_substring,
                     substring_of_fee.length() / 2);
    // Compare the byte arrays
    for (uint8_t i = 0; i < fee.length() / 2; i++) {
        EXPECT_EQ(bytes_corresponding_to_string[i],
                  bytes_corresponding_to_substring[i]) <<
        "At arrray index " << (unsigned int) i <<
        ", element of byte array created from String \"" << fee.c_str() <<
        "\", does not match the element of byte array created from \"" <<
        substring_of_fee.c_str() << "\".";
    }

    // Clean up
    free(bytes_corresponding_to_string);
    free(bytes_corresponding_to_substring);
}
TEST(StringUtilTest, hex_string_to_byte_array_conversion) {
    uint8_t size_of_byte_array = 0;

    uint8_t desired_number_of_bytes_to_be_copied = 0;
    uint8_t actual_number_of_bytes_copied = 0;

    bool prefer_lower_case = true;

    String ate_bad_f00d = String("8badf00d");
    size_of_byte_array = ate_bad_f00d.length() / 2;
    uint8_t* bytes_corresponding_to_string = (uint8_t*) malloc(
        sizeof(uint8_t) * size_of_byte_array);
    // copy the entire string into the byte array
    desired_number_of_bytes_to_be_copied = size_of_byte_array;
    actual_number_of_bytes_copied = HexStringToBytes(
        ate_bad_f00d,
        bytes_corresponding_to_string,
        desired_number_of_bytes_to_be_copied);

    EXPECT_EQ(desired_number_of_bytes_to_be_copied,
              actual_number_of_bytes_copied) <<
    "The function HexStringToBytes was unable to convert the string \"" <<
    ate_bad_f00d.c_str() << "\" into a byte array.";

    // If string is copied into byte array completely,
    // perform the reverse conversion and verify equality.
    if (desired_number_of_bytes_to_be_copied == actual_number_of_bytes_copied) {
        size_of_byte_array = actual_number_of_bytes_copied;

        String converted_string = BytesToHexString(
            bytes_corresponding_to_string,
            size_of_byte_array,
            prefer_lower_case);

        EXPECT_STREQ(ate_bad_f00d.c_str(), converted_string.c_str()) <<
        "The string \"" << ate_bad_f00d.c_str() <<
        "\" was converted into a byte array, which was again converted back "
        "to the string \"" << converted_string.c_str() << "\".";
    }

    // Clean up
    free(bytes_corresponding_to_string);
}

TEST(StringUtilTest, hex_string_to_byte_array_conversion_with_delimiter) {
    uint8_t size_of_byte_array = 0;

    uint8_t desired_number_of_bytes_to_be_copied = 0;
    uint8_t actual_number_of_bytes_copied = 0;

    bool prefer_lower_case = true;

    // String with a non-hex character and a delimiter.
    String bad_cafe = String("BA,D:,CA,FE");
    char separator = ',';
    // A non-exact upper-bound value for the size of byte array
    size_of_byte_array = bad_cafe.length() / 2;
    uint8_t* bytes_corresponding_to_string = (uint8_t*) malloc(
        sizeof(uint8_t) * size_of_byte_array);
    // Force the function to copy both 0xBA (valid) and OxD: (invalid)
    desired_number_of_bytes_to_be_copied = 2;
    actual_number_of_bytes_copied = HexStringToBytes(bad_cafe,
                                                     bytes_corresponding_to_string,
                                                     desired_number_of_bytes_to_be_copied,
                                                     separator);

    EXPECT_NE(actual_number_of_bytes_copied,
              desired_number_of_bytes_to_be_copied) <<
    "Tried to force the HexStringToBytes function to process "
    "the non-hex-digit character \':\' of String \"" <<
    bad_cafe.c_str() << "\" and expected it to be skipped.";

    // If it copied one byte as expected,
    // Perform the reverse conversion and verify
    if (1 == actual_number_of_bytes_copied) {
        size_of_byte_array = actual_number_of_bytes_copied;
        prefer_lower_case = false;

        String converted_string = BytesToHexString(
            bytes_corresponding_to_string,
            size_of_byte_array,
            prefer_lower_case,
            separator);

        String expected_string = bad_cafe.substr(0, 2);

        EXPECT_STREQ(expected_string.c_str(), converted_string.c_str()) <<
        "Expected the string \"" << converted_string.c_str() <<
        "\" created from the byte array, to match the original string \"" <<
        expected_string.c_str() << "\".";
    }

    // Clean up
    free(bytes_corresponding_to_string);
}

TEST(StringUtilTest, u8_hex_character_conversion_border_case) {
    // use a character outside the hex-digit characters
    EXPECT_EQ(255, CharToU8(':'));
    // use a number beyond the hex digits
    // Answer to the Ultimate Question of Life, The Universe, and Everything
    EXPECT_EQ('\0', U8ToChar(42));

}

TEST(StringUtilTest, u8_hex_character_conversion_stress) {
    // run through the whole range of hex digits
    uint8_t min_hex_digit =  0;
    uint8_t max_hex_digit = 15;

    for (uint8_t i = min_hex_digit; i <= max_hex_digit; i++) {
        EXPECT_EQ(i, CharToU8(U8ToChar(i)));
    }
}

TEST(StringUtilTest, int_to_string_conversion_stress) {
    uint16_t number_of_iterations = 1000;

    for (uint16_t i = 0; i < number_of_iterations; i++) {
        uint32_t some_u32 = Rand32();
        EXPECT_EQ(some_u32, StringToU32(U32ToString(some_u32)));

        int32_t some_i32 = (int32_t) some_u32;
        EXPECT_EQ(some_i32, StringToI32(I32ToString(some_i32)));

        uint64_t some_u64 = Rand64();
        EXPECT_EQ(some_u64, StringToU64(U64ToString(some_u64)));

        int64_t some_i64 = (int64_t) some_u64;
        EXPECT_EQ(some_i64, StringToI64(I64ToString(some_i64)));
    }
}
