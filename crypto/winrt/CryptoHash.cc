/**
 * @file CryptoHash.cc
 *
 * WinRT platform-specific implementation for hash function classes from Crypto.h
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

#include <assert.h>
#include <windows.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <Status.h>

using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;

#define QCC_MODULE "CRYPTO"

namespace qcc {

class Crypto_Hash::Context {
  public:

    Context(size_t digestSize) : digestSize(digestSize) { }

    CryptographicHash ^ hash;     // Only used for hash algorithms
    CryptographicKey ^ hmacKey;   // Only used for HMAC algorithms
    DataWriter ^ buf;             // Only used for HMAC algorithms

    size_t digestSize;

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

    Platform::String ^ algId;
    size_t digestSz;

    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        ctx = new Context(SHA1_SIZE);
        algId = MAC ? "HMAC_SHA1" : "SHA1";
        break;

    case qcc::Crypto_Hash::MD5:
        ctx = new Context(MD5_SIZE);
        algId = MAC ? "HMAC_MD5" : "MD5";
        break;

    case qcc::Crypto_Hash::SHA256:
        ctx = new Context(SHA256_SIZE);
        algId = MAC ? "HMAC_SHA256" : "SHA256";
        break;

    default:
        return ER_BAD_ARG_1;
    }

    // Open algorithm provider if required
    if (MAC) {
        MacAlgorithmProvider ^ provider = MacAlgorithmProvider::OpenAlgorithm(algId);
        if (provider == nullptr) {
            status = ER_CRYPTO_ERROR;
        } else {
            Platform::ArrayReference<uint8> inKey((uint8*)hmacKey, keyLen);
            ctx->hmacKey = provider->CreateKey(CryptographicBuffer::CreateFromByteArray(inKey));
            ctx->buf = ref new DataWriter();
        }
    } else {
        HashAlgorithmProvider ^ provider = HashAlgorithmProvider::OpenAlgorithm(algId);
        if (provider == nullptr) {
            status = ER_CRYPTO_ERROR;
        } else {
            ctx->hash = provider->CreateHash();
            ctx->buf = ref new DataWriter();
        }
    }
    if (status != ER_OK) {
        QCC_LogError(status, ("Failed to open algorithm provider"));
        delete ctx;
        ctx = NULL;
    } else {
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

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    QStatus status = ER_OK;

    if (!buf) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        Platform::ArrayReference<uint8> inBuf((uint8*)buf, bufSize);
        ctx->buf->WriteBytes(inBuf);
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

QStatus Crypto_Hash::GetDigest(uint8_t* digest, bool keepAlive)
{
    QStatus status = ER_OK;

    if (!digest) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        IBuffer ^ data = ctx->buf->DetachBuffer();
        IBuffer ^ result;
        if (MAC) {
            result = CryptographicEngine::Sign(ctx->hmacKey, data);
        } else {
            ctx->hash->Append(data);
            if (keepAlive) {
                ctx->buf->WriteBuffer(data);
            }
            result = ctx->hash->GetValueAndReset();
        }
        Platform::Array<uint8>^ resArray;
        CryptographicBuffer::CopyToByteArray(result, &resArray);
        memcpy(digest, resArray->Data, ctx->digestSize);
        if (!keepAlive) {
            initialized = false;
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

}
