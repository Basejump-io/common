/**
 * @file
 *
 * This file implements methods from the Environ class.
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
#include <map>

#include <qcc/winrt/utility.h>
#include <qcc/Debug.h>
#include <qcc/Environ.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Logger.h>

#include <Status.h>

#define QCC_MODULE "ENVIRON"

using namespace std;

namespace qcc {

Environ* Environ::GetAppEnviron(void)
{
    static Environ* env = NULL;      // Environment variable singleton.
    if (env == NULL) {
        env = new Environ();
    }
    return env;
}

qcc::String Environ::Find(const qcc::String& key, const char* defaultValue)
{
    qcc::String val;
    lock.Lock();
    val = vars[key];
    if (val.empty()) {
        if (strcmp(key.c_str(), "APPLICATIONDATA") == 0) {
            Platform::String ^ documentFolder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
            qcc::String strDocumentFolder = PlatformToMultibyteString(documentFolder);
            if (!strDocumentFolder.empty()) {
                Add(key.c_str(), strDocumentFolder.c_str());
                val = vars[key];
            }
        }
    }
    if (val.empty() && defaultValue) {
        val = defaultValue;
    }
    lock.Unlock();
    return val;
}

void Environ::Preload(const char* keyPrefix)
{
}

void Environ::Add(const qcc::String& key, const qcc::String& value)
{
    lock.Lock();
    vars[key] = value;
    lock.Unlock();
}

QStatus Environ::Parse(Source& source)
{
    QStatus status = ER_OK;
    lock.Lock();
    while (ER_OK == status) {
        qcc::String line;
        status = source.GetLine(line);
        if (ER_OK == status) {
            size_t endPos = line.find('#');
            if (qcc::String::npos != endPos) {
                line = line.substr(0, endPos);
            }
            size_t eqPos = line.find('=');
            if (qcc::String::npos != eqPos) {
                vars[Trim(line.substr(0, eqPos))] = Trim(line.substr(eqPos + 1));
            }
        }
    }
    lock.Unlock();
    return (ER_NONE == status) ? ER_OK : status;
}

}   /* namespace */
