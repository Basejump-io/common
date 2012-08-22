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
#include <qcc/Crypto.h>

using namespace qcc;

// The following test vectors were taken from
// RFC4648 - https://tools.ietf.org/html/rfc4648#section-10
static const char* raw_string_literal_array[] = {
    "f",
    "fo",
    "foo",
    "foob",
    "fooba",
    "foobar",
};

static const char* expected_base64_array[] = {
    "Zg==",
    "Zm8=",
    "Zm9v",
    "Zm9vYg==",
    "Zm9vYmE=",
    "Zm9vYmFy",
};

TEST(ASN1Test, encode_base64) {
    QStatus status = ER_FAIL;

    for (uint8_t i = 0; i < ArraySize(raw_string_literal_array); i++) {
        String raw_string = String(raw_string_literal_array[i]);
        String actual_encoded_base64;

        status = Crypto_ASN1::EncodeBase64(raw_string, actual_encoded_base64);
        EXPECT_EQ(ER_OK, status) <<
        "The function EncodeBase64 was unable to encode the string \"" <<
        raw_string.c_str() << "\" to Base64 format.";

        if (ER_OK == status) {
            String expected_base64 = String(expected_base64_array[i]);
            // qcc:Crypto_ASN1::EncodeBase64 calls LineBreak (inline),
            // which is a helper function for putting lines breaks
            // at the appropriate location.
            // Accordingly, we add '\n' to the expected value.
            expected_base64 = expected_base64.append('\n');
            EXPECT_STREQ(expected_base64.c_str(),
                         actual_encoded_base64.c_str()) <<
            "The string \"" << raw_string.c_str() << "\" was converted to "
            "Base64 format. The result \"" << actual_encoded_base64.c_str() <<
            "\" did not match the expected value \"" <<
            expected_base64.c_str() << "\".";
        }
    }
}

TEST(ASN1Test, decode_base64) {
    QStatus status = ER_FAIL;

    for (uint8_t i = 0; i < ArraySize(expected_base64_array); i++) {
        String base64_string = String(expected_base64_array[i]);
        String actual_decoded_string;

        status = Crypto_ASN1::DecodeBase64(base64_string,
                                           actual_decoded_string);
        EXPECT_EQ(ER_OK, status) <<
        "The function DecodeBase64 was unable to decode the string \"" <<
        base64_string.c_str() << "\".";

        if (ER_OK == status) {
            String expected_decoded_string = String(raw_string_literal_array[i]);
            EXPECT_STREQ(expected_decoded_string.c_str(),
                         actual_decoded_string.c_str()) <<
            "The string \"" << base64_string.c_str() << "\" was decoded from "
            "Base64 format. The result \"" << actual_decoded_string.c_str() <<
            "\" did not match the expected value \"" <<
            expected_decoded_string.c_str() << "\".";
        }
    }
}

TEST(ASN1Test, decode_base64_negative_test) {
    QStatus status = ER_FAIL;

    const char* quote_of_stephen_colbert =
        "Twenty-two astronauts were born in Ohio. What is it about your state "
        "that makes people want to flee the Earth?";

    String raw_string = String(quote_of_stephen_colbert);
    String actual_encoded_base64;

    status = Crypto_ASN1::EncodeBase64(raw_string, actual_encoded_base64);
    EXPECT_EQ(ER_OK, status) <<
    "The function EncodeBase64 was unable to encode the string \"" <<
    raw_string.c_str() << "\" to Base64 format.";

    if (ER_OK == status) {
        String improperly_encoded_base64;
        String actual_decoded_string;

        // 1. Size of Base64 encoded data must be a multiple of 4
        improperly_encoded_base64 = String("foo").append(actual_encoded_base64);
        status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                           actual_decoded_string);
        EXPECT_EQ(ER_FAIL, status) <<
        "The function DecodeBase64 should have rejected the improperly "
        "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
        "\" of length " << improperly_encoded_base64.length() <<
        ", which is not a multiple of 4.";

        // 2. The number of pad '=' characters must not exceed 2
        improperly_encoded_base64 = actual_encoded_base64.append("====");
        status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                           actual_decoded_string);
        EXPECT_EQ(ER_FAIL, status) <<
        "The function DecodeBase64 should have rejected the improperly "
        "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
        "\", which has more than two pad characters.";

        // 3. The Base64 encoded data must only contain characters from
        //    'The Base 64 Alphabet' (65-character subset of US-ASCII)
        //
        //    The Alphabet used for Base64 is:
        //    ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
        //
        //    Remaining ASCII printable, non-whitespace, characters
        String remaining_printable_ascii_characters =
            String("`~!@#$%^&*()-_[]{}\\|;:'\",<.>/?");

        for (size_t i = 0; i < remaining_printable_ascii_characters.length();
             i++) {
            improperly_encoded_base64 = actual_encoded_base64.insert(
                i, &remaining_printable_ascii_characters[i]);

            status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                               actual_decoded_string);
            EXPECT_EQ(ER_FAIL, status) <<
            "The function DecodeBase64 should have rejected the improperly "
            "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
            "\", which contains the character " <<
            remaining_printable_ascii_characters[i] << ", that is not in "
            "The Base 64 Alphabet.";
        }
    }
}

// Separate testcase for null string "", because of the way linebreaks
// are added by Crypto_ASN1::EncodeBase64.
// In particular, for the null string, no line breaks are added
// by Crypto_ASN1::EncodeBase64.
TEST(ASN1Test, encode_and_decode_base64_null_string) {
    QStatus status = ER_FAIL;

    String null_string = String("");
    String actual_encoded_base64;

    status = Crypto_ASN1::EncodeBase64(null_string, actual_encoded_base64);

    EXPECT_EQ(ER_OK, status) <<
    "The function EncodeBase64 was unable to encode the null string \"" <<
    null_string.c_str() << "\" to Base64 format.";

    if (ER_OK == status) {
        // The Base64 encoding of null string is null string itself.
        EXPECT_STREQ(null_string.c_str(), actual_encoded_base64.c_str()) <<
        "The null string \"" << null_string.c_str() << "\" was converted to "
        "Base64 format. The result \"" << actual_encoded_base64.c_str() <<
        "\" was NOT a null string.";
    }

    String actual_decoded_string;

    status = Crypto_ASN1::DecodeBase64(null_string, actual_decoded_string);
    EXPECT_EQ(ER_OK, status) <<
    "The function DecodeBase64 was unable to decode the null string \"" <<
    null_string.c_str() << "\".";

    if (ER_OK == status) {
        EXPECT_STREQ(null_string.c_str(), actual_decoded_string.c_str()) <<
        "The null string \"" << null_string.c_str() << "\" was decoded from "
        "Base64 format. The result was NOT a null string.";
    }
}

TEST(ASN1Test, encode_and_decode_base64_stress_test) {
    QStatus status = ER_FAIL;

    const char* quote_of_atticus_finch =
        "You never really understand a person until you consider things from "
        "his point of view, until you climb inside of his skin and "
        "walk around in it.";

    const char* quote_of_james_bond = "Bond. James Bond.";

    String raw_data = String(quote_of_atticus_finch);
    uint8_t number_of_rounds = 100;
    for (uint8_t i = 0; i < number_of_rounds; i++) {
        raw_data.append(' ');
        if (0 == Rand8() % 2) {
            raw_data.append(quote_of_atticus_finch,
                            strlen(quote_of_atticus_finch));
        } else {
            raw_data.append(quote_of_james_bond,
                            strlen(quote_of_james_bond));
        }

        String actual_encoded_base64;

        status = Crypto_ASN1::EncodeBase64(raw_data, actual_encoded_base64);
        EXPECT_EQ(ER_OK, status) <<
        "The function EncodeBase64 was unable to encode the string \"" <<
        raw_data.c_str() << "\" to Base64 format.";

        if (ER_OK == status) {
            String actual_decoded_string;
            status = Crypto_ASN1::DecodeBase64(actual_encoded_base64,
                                               actual_decoded_string);
            EXPECT_EQ(ER_OK, status) <<
            "The function DecodeBase64 was unable to decode the string \"" <<
            actual_encoded_base64.c_str() << "\".";

            if (ER_OK == status) {
                EXPECT_STREQ(raw_data.c_str(),
                             actual_decoded_string.c_str()) <<
                "The string \"" << raw_data.c_str() << "\" was encoded to "
                "Base64 format and decoded back. The decoded string \"" <<
                actual_decoded_string.c_str() <<
                "\" does NOT match the original.";
            }
        }
    }
}
