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
#include <qcc/RendezvousServerCertificateAuthority.h>

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

static SSL_CTX* sslCtx = NULL;
static qcc::Mutex ctxMutex;

SslSocket::SslSocket()
    : bio(NULL),
    x509(NULL),
    sourceEvent(&qcc::Event::neverSet),
    sinkEvent(&qcc::Event::neverSet)
{
    /* Initialize the global SSL context is this is the first SSL socket */
    if (!sslCtx) {
        ctxMutex.Lock();
        /* Check sslCtx again now that we have the mutex */
        if (!sslCtx) {
            SSL_library_init();
            SSL_load_error_strings();
            ERR_load_BIO_strings();
            OpenSSL_add_all_algorithms();
            sslCtx = SSL_CTX_new(SSLv23_client_method());
            if (sslCtx) {

            	  if (!SSL_CTX_load_verify_locations(sslCtx, "/home/padmapri/certs/ca.pem", NULL)) {

            	    QCC_LogError(ER_SSL_INIT, ("SslSocket::SslSocket(): Cannot initialize SSL trust store"));

            	    SSL_CTX_free(sslCtx);

            	    sslCtx = NULL;

            	  }

/*                X509_STORE* store = X509_STORE_new();
                SSL_CTX_set_cert_store(sslCtx, store);*/

                X509_STORE* myStore = SSL_CTX_get_cert_store(sslCtx);
                QCC_DbgPrintf(("SslSocket::SslSocket(): myStore = 0x%x", myStore));

                QStatus status = ImportPEM();
                if(status == ER_OK) {
                    int ret = X509_STORE_add_cert(myStore, issuer);
                    QCC_DbgPrintf(("SslSocket::SslSocket(): issuer ret = %d", ret));
                    QCC_LogError(ER_SSL_INIT, ("X509_STORE_add_cert: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));

                    ret = X509_STORE_add_cert(myStore, x509);
                    QCC_DbgPrintf(("SslSocket::SslSocket(): ret = %d", ret));
                    QCC_LogError(ER_SSL_INIT, ("X509_STORE_add_cert: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
                } else {
                	QCC_LogError(status, ("ImportPEM() failed"));
                }
            }
            if (!sslCtx) {
                QCC_LogError(ER_SSL_INIT, ("OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
            }

            /* SSL generates SIGPIPE which we don't want */
            signal(SIGPIPE, SIG_IGN);
        }
        ctxMutex.Unlock();
    }
}

SslSocket::~SslSocket() {
    Close();
}

QStatus SslSocket::Connect(const qcc::String hostName, uint16_t port)
{
    QStatus status = ER_OK;

    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    /* Sanity check */
    if (!sslCtx) {
        QCC_LogError(ER_SSL_INIT, ("SSL failed to initialize"));
        return ER_SSL_INIT;
    }

    /* Create the descriptor for this SSL socket */
    SSL* ssl;
    bio = BIO_new_ssl_connect(sslCtx);
    QCC_LogError(ER_SSL_INIT, ("BIO_new_ssl_connect: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));

    if (bio) {
        /* Set SSL modes */
        BIO_get_ssl(bio, &ssl);
        QCC_LogError(ER_SSL_INIT, ("BIO_get_ssl: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
        QCC_LogError(ER_SSL_INIT, ("SSL_set_mode: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));

        /* Set destination host name and port */
        int intPort = (int) port;
        BIO_set_conn_hostname(bio, hostName.c_str());
        QCC_LogError(ER_SSL_INIT, ("BIO_set_conn_hostname: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        BIO_set_conn_int_port(bio, &intPort);
        QCC_LogError(ER_SSL_INIT, ("BIO_set_conn_int_port: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));

        /* Connect to destination */
        if (0 < BIO_do_connect(bio)) {
        	QCC_LogError(ER_SSL_INIT, ("BIO_do_connect: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
            /* Verify the certificate */
        	long int ret = SSL_get_verify_result(ssl);
        	QCC_LogError(ER_SSL_INIT, ("SSL_get_verify_result: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        	QCC_DbgPrintf(("SslSocket::Connect(): ret = %d", ret));
            if (ret != X509_V_OK) {
                status = ER_SSL_VERIFY;
            }
        } else {
        	QCC_LogError(ER_SSL_INIT, ("BIO_do_connect: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
            status = ER_SSL_CONNECT;
        }
    } else {
        status = ER_SSL_CONNECT;
    }

    /* Set the events */
    if (ER_OK == status) {
        int fd = BIO_get_fd(bio, 0);
        QCC_LogError(ER_SSL_INIT, ("BIO_get_fd: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        sourceEvent = new qcc::Event(fd, qcc::Event::IO_READ, false);
        sinkEvent = new qcc::Event(fd, qcc::Event::IO_WRITE, false);
    }

    /* Cleanup on error */
    if (bio && (ER_OK != status)) {
        BIO_free_all(bio);
        QCC_LogError(ER_SSL_INIT, ("BIO_free_all: OpenSSL error is \"%s\"", ERR_reason_error_string(ERR_get_error())));
        bio = NULL;
    }

    /* Log any errors */
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to connect SSL socket"));
    }

    return status;
}

void SslSocket::Close()
{
    if (bio) {
        BIO_free_all(bio);
        bio = NULL;
    }

    if (sourceEvent != &qcc::Event::neverSet) {
        delete sourceEvent;
        sourceEvent = &qcc::Event::neverSet;
    }
    if (sinkEvent != &qcc::Event::neverSet) {
        delete sinkEvent;
        sinkEvent = &qcc::Event::neverSet;
    }
}

QStatus SslSocket::PullBytes(void*buf, size_t reqBytes, size_t& actualBytes)
{
    QStatus status;
    int r;

    if (!bio) {
        return ER_FAIL;
    }

    r = BIO_read(bio, buf, reqBytes);

    if (0 == r) {
        actualBytes = r;
        status = ER_NONE;
    } else if (0 > r) {
        status = ER_FAIL;
        QCC_LogError(status, ("BIO_read failed with error=%d", ERR_get_error()));
    } else {
        actualBytes = r;
        status = ER_OK;
    }
    return status;
}

QStatus SslSocket::PushBytes(void*buf, size_t numBytes, size_t& numSent)
{
    QStatus status;
    int s;

    if (!bio) {
        return ER_FAIL;
    }

    s = BIO_write(bio, buf, numBytes);

    if (0 < s) {
        status = ER_OK;
        numSent = s;
    } else {
        status = ER_FAIL;
        QCC_LogError(status, ("BIO_write failed with error=%d", ERR_get_error()));
    }

    return status;
}

QStatus SslSocket::ImportPEM()
{
    ERR_load_crypto_strings();
    QStatus status = ER_CRYPTO_ERROR;
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_write(bio, x509RendezvousServerCert, sizeof(x509RendezvousServerCert));
    x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (x509) {
        status = ER_OK;
    }

    status = ER_CRYPTO_ERROR;
    bio = BIO_new(BIO_s_mem());
    BIO_write(bio, x509IssuerCert, sizeof(x509IssuerCert));
    issuer = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (issuer) {
        status = ER_OK;
    }

    QCC_DbgPrintf(("SslSocket::ImportPEM(): status = %s", QCC_StatusText(status)));

    return status;
}

}  /* namespace */
