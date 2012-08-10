/**
 * @file
 *
 * SSL stream-based socket implementation for WinRT
 *
 */

/******************************************************************************
 * Copyright 2012 Qualcomm Innovation Center, Inc.
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
#include <qcc/IPAddress.h>
#include <qcc/SslSocket.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/RendezvousServerRootCertificate.h>
#include <qcc/winrt/SslSocketWrapper.h>
#include <qcc/winrt/utility.h>

#define QCC_MODULE  "SSL"

using namespace std;

namespace qcc {

struct SslSocket::Internal {
    Internal() {
        socket = ref new qcc::winrt::SslSocketWrapper();
    }
    uint32_t _lastError;
    Crypto_RSA rootCert;
    Crypto_RSA rootCACert;
    qcc::winrt::SslSocketWrapper ^ socket;
};

SslSocket::SslSocket(String host) :
    internal(new Internal()),
    sourceEvent(&qcc::Event::neverSet),
    sinkEvent(&qcc::Event::neverSet),
    Host(host),
    sock(-1)
{
    /* Convert the PEM-encoded root certificate defined by ServerRootCertificate into the X509 format*/
    QStatus status = ImportPEM();
    if (status == ER_OK) {
        /// Add the certificate to the current certificate storage
        /// NOTE: There is no WinRT API to do this, so the retrieval is currently useless
    } else {
        QCC_LogError(status, ("SslSocket::SslSocket(): ImportPEM() failed"));
    }

}

SslSocket::~SslSocket() {
    Close();
    delete internal;
}

QStatus SslSocket::Connect(const qcc::String hostName, uint16_t port)
{
    QStatus status = ER_FAIL;

    while (true) {

        // TODO: stop hardcoding family and socket type.
        status = (QStatus) internal->socket->Init(qcc::winrt::AddressFamily::QCC_AF_INET,
                                                  qcc::winrt::SocketType::QCC_SOCK_STREAM);
        if (status != ER_OK) {
            break;
        }

        Platform::String ^ strRemoteAddr = MultibyteToPlatformString(hostName.c_str());
        if (nullptr == strRemoteAddr) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)internal->socket->Connect(strRemoteAddr, port);
        if (ER_OK == status) {
            sock = (SocketFd) reinterpret_cast<void*>(internal->socket->_sw);
            sourceEvent = new qcc::Event(sock, qcc::Event::IO_READ, false);
            sinkEvent = new qcc::Event(sock, qcc::Event::IO_WRITE, false);
        }
        break;
    }

    internal->_lastError = (uint32_t)status;
    return status;
}

void SslSocket::Close()
{
    QStatus status = ER_FAIL;

    while (true) {
        status = (QStatus)(int)internal->socket->Close();
        break;
    }

    internal->_lastError = (uint32_t)status;

    if (sourceEvent != &qcc::Event::neverSet) {
        delete sourceEvent;
        sourceEvent = &qcc::Event::neverSet;
    }
    if (sinkEvent != &qcc::Event::neverSet) {
        delete sinkEvent;
        sinkEvent = &qcc::Event::neverSet;
    }

    sock = -1;
}

QStatus SslSocket::PullBytes(void*buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    QStatus status = ER_FAIL;

    while (true) {
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, reqBytes);
        Platform::Array<int> ^ tempReceived = ref new Platform::Array<int>(1);
        if (nullptr == tempReceived) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)internal->socket->Recv(arrRef, reqBytes, tempReceived);
        actualBytes = tempReceived[0];
        break;
    }

    internal->_lastError = (uint32_t)status;
    return status;
}

QStatus SslSocket::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    QStatus status = ER_FAIL;

    while (true) {
        Platform::ArrayReference<uint8> arrRef((unsigned char*)buf, numBytes);
        Platform::Array<int> ^ tempSent = ref new Platform::Array<int>(1);
        if (nullptr == tempSent) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        status = (QStatus)(int)internal->socket->Send(arrRef, numBytes, tempSent);
        numSent = tempSent[0];
        break;
    }

    internal->_lastError = (uint32_t)status;
    return status;
}

QStatus SslSocket::ImportPEM()
{
    /* Initialize the appropriate root certificate to be used for HTTPS connection */
    QStatus status = InitializeServerRootCertificate(Host);

    if (status != ER_OK) {
        QCC_LogError(status, ("SslSocket::ImportPEM(): %s", QCC_StatusText(status)));
    }

    status = ER_CRYPTO_ERROR;
    QCC_DbgPrintf(("SslSocket::ImportPEM(): Server = %s Certificate = %s", Host.c_str(), String(ServerRootCertificate).c_str()));

    status = internal->rootCert.ImportPEM(qcc::String(ServerRootCertificate));
    if (status != ER_OK) {
        QCC_LogError(status, ("SslSocket::ImportPEM(): ServerRootCertificate invalid %s", QCC_StatusText(status)));
    }

    // load the CA certificate as well, to enable verification
    QCC_DbgPrintf(("SslSocket::ImportPEM(): Server = %s Certificate = %s", Host.c_str(), String(ServerCACertificate).c_str()));

    status = internal->rootCert.ImportPEM(qcc::String(ServerCACertificate));
    if (status != ER_OK) {
        QCC_LogError(status, ("SslSocket::ImportPEM(): RendezvousServerCACertificate invalid %s", QCC_StatusText(status)));
    }

    QCC_DbgPrintf(("SslSocket::ImportPEM(): status = %s", QCC_StatusText(status)));

    return status;
}

}  /* namespace */
