/**
 * @file
 *
 * SSL stream-based socket implementation for Linux
 *
 */

/******************************************************************************
 * Copyright 2009,2012 Qualcomm Innovation Center, Inc.
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

#include <signal.h>
#include <qcc/platform.h>
#include <qcc/IPAddress.h>
#include <qcc/Stream.h>
#include <qcc/SslSocket.h>
#include <qcc/Config.h>
#include <qcc/Mutex.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/RendezvousServerRootCertificate.h>

#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "Status.h"


#define QCC_MODULE  "SSL"

using namespace std;

namespace qcc {

static int32_t refCount = 0;

static SSL_CTX* sslCtx = NULL;

struct SslSocket::Internal {
    Internal() : bio(NULL), rootCert(NULL), rootCACert(NULL) { }

    BIO* bio;                      /**< SSL socket descriptor for OpenSSL */
    X509* rootCert;                /**< Hard-coded Root Certificate */
    X509* rootCACert;              /**< Hard-coded Root CA Certificate */
};

SslSocket::SslSocket(String host) :
    internal(new Internal()),
    sourceEvent(&qcc::Event::neverSet),
    sinkEvent(&qcc::Event::neverSet),
    Host(host),
    sock(-1)
{
    /*
     * Protect open ssl
     */
    Crypto_ScopedLock lock;

    /* Initialize the global SSL context is this is the first SSL socket */
    if (!sslCtx) {

        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        sslCtx = SSL_CTX_new(SSLv23_client_method());

        if (sslCtx) {

            /* Set up our own trust store */
            X509_STORE* store = X509_STORE_new();

            /* Replace the certificate verification storage of sslCtx with store */
            SSL_CTX_set_cert_store(sslCtx, store);

            /* Get a reference to the current certificate verification storage */
            X509_STORE* sslCtxstore = SSL_CTX_get_cert_store(sslCtx);

            /* Convert the PEM-encoded root certificate defined by RendezvousServerRootCertificate in to the X509 format*/
            QStatus status = ImportPEM();
            if (status == ER_OK) {

                /* Add the root certificate to the current certificate verification storage */
                if (X509_STORE_add_cert(sslCtxstore, internal->rootCert) != 1) {
                    QCC_LogError(ER_SSL_INIT, ("SslSocket::SslSocket(): X509_STORE_add_cert: OpenSSL error is \"%s\"",
                                               ERR_reason_error_string(ERR_get_error())));
                }

                if (internal->rootCACert != NULL) {
                    /* Add the root certificate to the current certificate verification storage */
                    if (X509_STORE_add_cert(sslCtxstore, internal->rootCACert) != 1) {
                        QCC_LogError(ER_SSL_INIT, ("SslSocket::SslSocket(): X509_STORE_add_cert: OpenSSL error is \"%s\"",
                                                   ERR_reason_error_string(ERR_get_error())));
                    }
                }

                /* Set the default verify paths for the SSL context */
                if (SSL_CTX_set_default_verify_paths(sslCtx) != 1) {
                    QCC_LogError(ER_SSL_INIT, ("SslSocket::SslSocket(): SSL_CTX_set_default_verify_paths: OpenSSL error is \"%s\"",
                                               ERR_reason_error_string(ERR_get_error())));
                }

            } else {
                QCC_LogError(status, ("SslSocket::SslSocket(): ImportPEM() failed"));
            }

            /* SSL generates SIGPIPE which we don't want */
            signal(SIGPIPE, SIG_IGN);

        } else {
            DecrementAndFetch(&refCount);
            QCC_LogError(ER_SSL_INIT, ("SslSocket::SslSocket(): OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        }
    }
}

SslSocket::~SslSocket() {
    Close();
    delete internal;
}

QStatus SslSocket::Connect(const qcc::String hostName, uint16_t port)
{
    /*
     * Protect the open ssl APIs.
     */
    Crypto_ScopedLock lock;

    QStatus status = ER_OK;

    /* Sanity check */
    if (!sslCtx) {
        QCC_LogError(ER_SSL_INIT, ("SslSocket::Connect(): SSL failed to initialize"));
        return ER_SSL_INIT;
    }

    /* Create the descriptor for this SSL socket */
    SSL* ssl;
    internal->bio = BIO_new_ssl_connect(sslCtx);

    if (internal->bio) {
        /* Set SSL modes */
        BIO_get_ssl(internal->bio, &ssl);
        SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

        /* Set destination host name and port */
        int intPort = (int) port;
        BIO_set_conn_hostname(internal->bio, hostName.c_str());
        BIO_set_conn_int_port(internal->bio, &intPort);

        /* Connect to destination */
        if (0 < BIO_do_connect(internal->bio)) {
            /* Verify the certificate */
            int verifyResult = SSL_get_verify_result(ssl);
            if (verifyResult != X509_V_OK) {
                status = ER_SSL_VERIFY;
                QCC_LogError(status, ("SslSocket::Connect(): SSL_get_verify_result: returns %d OpenSSL error is \"%s\"", verifyResult, ERR_reason_error_string(ERR_get_error())));
            }
        } else {
            QCC_LogError(ER_SSL_INIT, ("SslSocket::Connect(): BIO_do_connect: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
            status = ER_SSL_CONNECT;
        }
    } else {
        status = ER_SSL_CONNECT;
    }

    /* Set the events */
    if (ER_OK == status) {
        sock = BIO_get_fd(internal->bio, 0);
        sourceEvent = new qcc::Event(sock, qcc::Event::IO_READ, false);
        sinkEvent = new qcc::Event(sock, qcc::Event::IO_WRITE, false);
    }

    /* Cleanup on error */
    if (internal->bio && (ER_OK != status)) {
        BIO_free_all(internal->bio);
        internal->bio = NULL;
    }

    /* Log any errors */
    if (ER_OK != status) {
        QCC_LogError(status, ("SslSocket::Connect(): Failed to connect SSL socket"));
    }

    return status;
}

void SslSocket::Close()
{
    if (internal->bio) {
        BIO_free_all(internal->bio);
        internal->bio = NULL;
    }

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
    QStatus status;
    int r;

    if (!internal->bio) {
        return ER_FAIL;
    }

    r = BIO_read(internal->bio, buf, reqBytes);

    if (0 == r) {
        actualBytes = r;
        status = ER_NONE;
    } else if (0 > r) {
        status = ER_FAIL;
        QCC_LogError(status, ("SslSocket::PullBytes(): BIO_read failed with error=%d", ERR_get_error()));
    } else {
        actualBytes = r;
        status = ER_OK;
    }
    return status;
}

QStatus SslSocket::PushBytes(const void* buf, size_t numBytes, size_t& numSent)
{
    QStatus status;
    int s;

    if (!internal->bio) {
        return ER_FAIL;
    }

    s = BIO_write(internal->bio, buf, numBytes);

    if (0 < s) {
        status = ER_OK;
        numSent = s;
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("SslSocket::PushBytes(): BIO_write failed with error=%d", ERR_get_error()));
    }

    return status;
}

QStatus SslSocket::ImportPEM()
{
    /*
     * Protect the open ssl APIs.
     */
    Crypto_ScopedLock lock;

    /* Initialize the appropriate root certificate to be used for HTTPS connection */
    QStatus status = InitializeServerRootCertificate(Host);

    if (status != ER_OK) {
        QCC_LogError(status, ("SslSocket::ImportPEM(): %s", QCC_StatusText(status)));
    }

    ERR_load_crypto_strings();
    status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    QCC_DbgPrintf(("SslSocket::ImportPEM(): Server = %s Certificate = %s", Host.c_str(), String(RendezvousServerRootCertificate).c_str()));
    BIO_write(bio, RendezvousServerRootCertificate, String(RendezvousServerRootCertificate).size());
    internal->rootCert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (internal->rootCert) {
        status = ER_OK;
    }

    // load the CA certificate as well, to enable verification
    bio = BIO_new(BIO_s_mem());
    QCC_DbgPrintf(("SslSocket::ImportPEM(): Server = %s Certificate = %s", Host.c_str(), String(RendezvousServerCACertificate).c_str()));
    BIO_write(bio, RendezvousServerCACertificate, String(RendezvousServerCACertificate).size());
    internal->rootCACert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);

    QCC_DbgPrintf(("SslSocket::ImportPEM(): status = %s", QCC_StatusText(status)));

    return status;
}

}  /* namespace */
