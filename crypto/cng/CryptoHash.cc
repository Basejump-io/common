/**
 * @file CryptoHash.cc
 *
 * Windows platform-specific implementation for hash function classes from Crypto.h
 */

/******************************************************************************
 * Copyright 2011, Qualcomm Innovation Center, Inc.
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

#include <assert.h>
#include <windows.h>
#include <bcrypt.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {

// Cache of open algorithm handles
static BCRYPT_ALG_HANDLE algHandles[3][2] = { 0 };

class Crypto_Hash::Context {
  public:

    Context(size_t digestSize) : digestSize(digestSize), handle(0), hashObj(NULL) { }

    ~Context() {
        if (handle) {
            BCryptDestroyHash(handle);
        }
        delete [] hashObj;
    }

    size_t digestSize;
    BCRYPT_HASH_HANDLE handle;
    uint8_t* hashObj;
    DWORD hashObjLen;

};

QStatus Crypto_Hash::Init(Algorithm alg, const uint8_t* hmacKey, size_t keyLen)
{
    QStatus status = ER_OK;

    if (ctx) {
        delete ctx;
        ctx = NULL;
        initialized = false;
    }

    MAC = hmacKey != NULL;

    if (MAC && (keyLen == 0)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("HMAC key length cannot be zero"));
        return status;
    }

    LPCWSTR algId;
    size_t digestSz;

    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        ctx = new Context(SHA1_SIZE);
        algId = BCRYPT_SHA1_ALGORITHM;
        break;

    case qcc::Crypto_Hash::MD5:
        ctx = new Context(MD5_SIZE);
        algId = BCRYPT_MD5_ALGORITHM;
        break;

    case qcc::Crypto_Hash::SHA256:
        ctx = new Context(SHA256_SIZE);
        algId = BCRYPT_SHA256_ALGORITHM;
        break;

    default:
        return ER_BAD_ARG_1;
    }

    // Open algorithm provider if required
    if (!algHandles[alg][MAC]) {
        if (BCryptOpenAlgorithmProvider(&algHandles[alg][MAC], algId, MS_PRIMITIVE_PROVIDER, MAC ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0) < 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to open algorithm provider"));
            delete ctx;
            ctx = NULL;
            return status;
        }
    }

    // Get length of hash object and allocate the object
    DWORD got;
    BCryptGetProperty(algHandles[alg][MAC], BCRYPT_OBJECT_LENGTH, (PBYTE)&ctx->hashObjLen, sizeof(DWORD), &got, 0);
    ctx->hashObj = new uint8_t[ctx->hashObjLen];

    if (BCryptCreateHash(algHandles[alg][MAC], &ctx->handle, ctx->hashObj, ctx->hashObjLen, (PUCHAR)hmacKey, (ULONG)keyLen, 0) < 0) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to create hash"));
        delete ctx;
        ctx = NULL;
    }

    if (status == ER_OK) {
        initialized = true;
    }
    return status;
}

Crypto_Hash::~Crypto_Hash(void)
{
    if (ctx) {
        delete ctx;
    }
}

Crypto_Hash& Crypto_Hash::operator=(const Crypto_Hash& other)
{
    QStatus status;

    if (ctx) {
        delete ctx;
    }
    if (!other.initialized) {
        ctx = NULL;
        initialized = false;
        return *this;
    }

    ctx = new Context(other.ctx->digestSize);

    ctx->hashObjLen = other.ctx->hashObjLen;
    ctx->hashObj = new uint8_t[ctx->hashObjLen];

    MAC = other.MAC;
    initialized = other.initialized;

    if (BCryptDuplicateHash(other.ctx->handle, &ctx->handle, ctx->hashObj, ctx->hashObjLen, 0) < 0) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to create hash"));
        delete ctx;
        ctx = NULL;
        initialized = false;
    }
    return *this;
}

Crypto_Hash::Crypto_Hash(const Crypto_Hash& other)
{
    ctx = NULL;
    *this = other;
}

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    QStatus status = ER_OK;

    if (!buf) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (BCryptHashData(ctx->handle, (PUCHAR)buf, bufSize, 0) < 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Updating hash digest"));
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

QStatus Crypto_Hash::Update(const qcc::String& str)
{
    return Update((const uint8_t*)str.data(), str.size());
}

QStatus Crypto_Hash::GetDigest(uint8_t* digest)
{
    QStatus status = ER_OK;

    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (BCryptFinishHash(ctx->handle, digest, ctx->digestSize, 0) < 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Finalizing hash digest"));
        }
        initialized = false;
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

}
