/**
 * @file adapterUtil.cpp
 *
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

#include <string>
#include <list>

#include <qcc/AdapterUtil.h>
#include <qcc/NetInfo.h>
#include <qcc/IfConfig.h>

using namespace std;

/** @internal */
#define QCC_MODULE "ADAPTERUTIL"

namespace qcc {

AdapterUtil* AdapterUtil::singleton = NULL;
qcc::Mutex qcc::AdapterUtil::singletonLock;

AdapterUtil::~AdapterUtil(void) {
}

QStatus AdapterUtil::ForceUpdate()
{
    std::vector<qcc::IfConfigEntry> entries;
    IfConfig(entries);
    for (std::vector<IfConfigEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {

        if (((it->m_flags & qcc::IfConfigEntry::UP) == 0) ||
            ((it->m_flags & qcc::IfConfigEntry::LOOPBACK) != 0)) {
            continue;
        }
        NetInfo netInfo;
        netInfo.name = it->m_name;
        netInfo.addr = IPAddress(it->m_addr);
        netInfo.mtu = it->m_mtu;
        netInfo.isVPN = false;

        QCC_DbgPrintf(("Interface: name=%s  addr=%s  MTU=%u",
                       netInfo.name.c_str(), netInfo.addr.ToString().c_str(), netInfo.mtu));

        interfaces.push_back(netInfo);
    }
    return ER_OK;
}


bool AdapterUtil::IsVPN(IPAddress addr)
{
    return false;
}

}   /* namespace */
