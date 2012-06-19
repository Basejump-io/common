/**
 * @file
 * A mechanism to get network interface configurations a la Unix/Linux ifconfig.
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

#include <list>

#include <qcc/Event.h>
#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Socket.h>
#include <qcc/winrt/utility.h>
#include <qcc/IfConfig.h>

#include <ctxtcall.h>
#include <ppltasks.h>

#define QCC_MODULE "IFCONFIG"
#define AF_INET
#define AF_INET6

using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace qcc {

QStatus IfConfig(std::vector<IfConfigEntry>& entries)
{
    QStatus status = ER_OK;

    QCC_DbgPrintf(("IfConfig(): The WinRT way"));

    while (true) {
        IVectorView<HostName ^> ^ localEntries = nullptr;
        try {
            localEntries = NetworkInformation::GetHostNames();
            if (nullptr == localEntries) {
                status = ER_OS_ERROR;
                break;
            }
        } catch (...) {
            status = ER_FAIL;
            break;
        }

        for (int i = 0; i < localEntries->Size; i++) {
            HostName ^ tempEntry = localEntries->GetAt(i);
            if (nullptr != tempEntry && nullptr != tempEntry->IPInformation && nullptr != tempEntry->IPInformation->NetworkAdapter) {
                // Now, we look for interfaces which have a connection profile and are connected
                IAsyncOperation<ConnectionProfile ^> ^ op = nullptr;
                try {
                    op = tempEntry->IPInformation->NetworkAdapter->GetConnectedProfileAsync();
                    concurrency::task<ConnectionProfile ^> getProfileTask(op);
                    getProfileTask.wait();
                    ConnectionProfile ^ profile = op->GetResults();
                    if (nullptr != profile && profile->GetNetworkConnectivityLevel() != NetworkConnectivityLevel::None) {
                        // TODO: We should probably be smarter about which iana If types we suck up
                        IfConfigEntry entry;
                        qcc::String strProfileName = PlatformToMultibyteString(profile->ProfileName);
                        if (nullptr != profile->ProfileName && strProfileName.empty()) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        entry.m_name = strProfileName;
                        entry.m_flags = IfConfigEntry::UP | IfConfigEntry::MULTICAST;
                        qcc::String strAddr = PlatformToMultibyteString(tempEntry->CanonicalName);
                        if (nullptr != tempEntry->CanonicalName && strAddr.empty()) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        entry.m_addr = strAddr;
                        if (strAddr.find_first_of(':') != strAddr.npos) {
                            entry.m_family = QCC_AF_INET6;
                        } else {
                            entry.m_family = QCC_AF_INET;
                        }
                        entry.m_mtu = 1500;                         // ping -f -l 1472 tells me this size is ok
                        entry.m_prefixlen = static_cast<uint32_t>(-1);
                        entry.m_index = static_cast<uint32_t>(-1);

                        entries.push_back(entry);
                    }
                } catch (...) {
                    qcc::String strAddr = PlatformToMultibyteString(tempEntry->CanonicalName);
                    QCC_DbgPrintf(("IfConfig(): Failed to get configuration for adapter with address %s", strAddr.c_str()));
                    // Just continue
                }
            }
        }

        if (ER_OK != status) {
            break;
        }
        break;
    }

    if (ER_OK != status) {
        QCC_DbgPrintf(("IfConfig(): Failed to get adapter configuration"));
    }

    return status;
}

} // namespace qcc
