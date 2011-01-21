/**
 * @file
 *
 * Sink/Source wrapper FILE operations for windows
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

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

#include <windows.h>

#include <qcc/FileStream.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE  "STREAM"


static void ReSlash(qcc::String& inStr)
{
    size_t pos = inStr.find_first_of("/");
    while (pos != qcc::String::npos) {
        inStr[pos] = '\\';
        pos = inStr.find_first_of("/", pos + 1);
    }
}

FileSource::FileSource(qcc::String fileName) : handle(INVALID_HANDLE_VALUE), event(0, 0), ownsHandle(true)
{
    ReSlash(fileName);
    handle = CreateFileA(fileName.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         INVALID_HANDLE_VALUE);

    if (INVALID_HANDLE_VALUE == handle) {
        QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_READ) %s failed (%d)", fileName.c_str(), GetLastError()));
    }
}

FileSource::FileSource() : handle(INVALID_HANDLE_VALUE), event(0, 0), ownsHandle(false)
{
    handle = GetStdHandle(STD_INPUT_HANDLE);

    if (NULL == handle) {
        QCC_LogError(ER_OS_ERROR, ("GetStdHandle failed (%d)", GetLastError()));
    }
}

FileSource::~FileSource()
{
    if (ownsHandle && (INVALID_HANDLE_VALUE != handle)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus FileSource::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    if (INVALID_HANDLE_VALUE == handle) {
        return ER_INIT_FAILED;
    }

    DWORD readBytes;
    BOOL ret = ReadFile(handle, buf, reqBytes, &readBytes, NULL);

    if (ret) {
        actualBytes = readBytes;
        return ((0 < reqBytes) && (0 == readBytes)) ? ER_NONE : ER_OK;
    } else {
        event.ResetTime(Event::WAIT_FOREVER, 0);
        DWORD error = GetLastError();
        if (ERROR_HANDLE_EOF == error) {
            actualBytes = 0;
            return ER_NONE;
        } else {
            QCC_LogError(ER_FAIL, ("ReadFile returned error (%d)", error));
            return ER_FAIL;
        }
    }
}

FileSink::FileSink(qcc::String fileName, Mode mode) : handle(INVALID_HANDLE_VALUE), event(Event::alwaysSet), ownsHandle(true)
{
    ReSlash(fileName);

    DWORD attributes;
    switch (mode) {
    case PRIVATE:
        attributes = FILE_ATTRIBUTE_HIDDEN;
        break;

    case WORLD_READABLE:
    case WORLD_WRITABLE:
        attributes = FILE_ATTRIBUTE_NORMAL;
        break;

    default:
        QCC_LogError(ER_BAD_ARG_2, ("Invalid mode"));
        return;
    }

    /* Compress leading slashes - we're not going to handle UNC paths */
    size_t skip = 0;
    while ('\\' == fileName[skip]) {
        ++skip;
    }
    skip = (skip > 0) ? (skip - 1) : 0;

    /* Create the intermediate directories */
    size_t begin = skip;
    for (size_t end = fileName.find('\\', begin); end != String::npos; end = fileName.find('\\', begin)) {

        /* Skip consecutive slashes */
        if (begin == end) {
            ++begin;
            continue;
        }

        /* Get the directory path */
        String p = fileName.substr(skip, end);

        /* Only try to create the directory if it doesn't already exist */
        if (CreateDirectoryA(p.c_str(), NULL)) {
            if (!SetFileAttributesA(p.c_str(), attributes)) {
                QCC_LogError(ER_OS_ERROR, ("SetFileAttributes() %s failed with (%d)", p.c_str(), GetLastError()));
                return;
            }
        } else if (ERROR_ALREADY_EXISTS != GetLastError()) {
            QCC_LogError(ER_OS_ERROR, ("CreateDirectory() %s failed with (%d)", p.c_str(), GetLastError()));
            return;
        }
        begin = end + 1;
    }

    /* Create and open the file */
    handle = CreateFileA(fileName.substr(skip).c_str(),
                         GENERIC_WRITE,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_ALWAYS,
                         attributes,
                         INVALID_HANDLE_VALUE);

    if (INVALID_HANDLE_VALUE == handle) {
        QCC_LogError(ER_OS_ERROR, ("CreateFile(GENERIC_WRITE) %s failed (%d)", fileName.c_str(), GetLastError()));
    }
}

FileSink::FileSink() : handle(INVALID_HANDLE_VALUE), event(Event::alwaysSet), ownsHandle(false)
{
    handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (NULL == handle) {
        QCC_LogError(ER_OS_ERROR, ("GetStdHandle failed (%d)", GetLastError()));
    }
}

FileSink::~FileSink() {
    if (INVALID_HANDLE_VALUE != handle) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

QStatus FileSink::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{

    if (INVALID_HANDLE_VALUE == handle) {
        return ER_INIT_FAILED;
    }

    DWORD writeBytes;
    BOOL ret = WriteFile(handle, buf, numBytes, &writeBytes, NULL);

    if (ret) {
        numSent = writeBytes;
        return ER_OK;
    } else {
        event.ResetTime(Event::WAIT_FOREVER, 0);
        QCC_LogError(ER_FAIL, ("WriteFile failed. error=%d", GetLastError()));
        return ER_FAIL;
    }
}



