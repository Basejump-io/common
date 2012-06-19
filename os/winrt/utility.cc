/**
 * @file
 *
 * Utility functions for WinRT
 */

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

#include <windows.h>
#include <assert.h>
#include <cstdlib>

#include <Status.h>

#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <Status.h>

#define QCC_MODULE "UTILITY"

using namespace Windows::Foundation;

wchar_t* MultibyteToWideString(const char* str)
{
    wchar_t* buffer = NULL;

    while (true) {
        if (NULL == str) {
            break;
        }
        size_t charLen = mbstowcs(NULL, str, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new wchar_t[charLen];
        if (NULL == buffer) {
            break;
        }
        if (mbstowcs(buffer, str, charLen) < 0) {
            break;
        }
        break;
    }

    return buffer;
}

Platform::String ^ MultibyteToPlatformString(const char* str)
{
    wchar_t* buffer = NULL;
    Platform::String ^ pstr = nullptr;

    while (true) {
        if (NULL == str) {
            break;
        }
        size_t charLen = mbstowcs(NULL, str, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new wchar_t[charLen];
        if (NULL == buffer) {
            break;
        }
        if (mbstowcs(buffer, str, charLen) < 0) {
            break;
        }
        pstr = ref new Platform::String(buffer);
        break;
    }

    if (NULL != buffer) {
        delete [] buffer;
        buffer = NULL;
    }

    return pstr;
}


qcc::String PlatformToMultibyteString(Platform::String ^ str)
{
    char* buffer = NULL;
    const wchar_t* wstr = str->Data();
    qcc::String result = "";

    while (true) {
        if (nullptr == str) {
            break;
        }
        size_t charLen = wcstombs(NULL, wstr, 0);
        if (charLen < 0) {
            break;
        }
        ++charLen;
        buffer = new char[charLen];
        if (NULL == buffer) {
            break;
        }
        if (wcstombs(buffer, wstr, charLen) < 0) {
            break;
        }
        result = buffer;
        break;
    }

    if (NULL != buffer) {
        delete [] buffer;
        buffer = NULL;
    }

    return result;
}
