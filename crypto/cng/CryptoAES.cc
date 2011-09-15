/**
 * @file CryptoAES.cc
 *
 * Class for AES block encryption/decryption this wraps the Windows CNG APIs
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

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Crypto.h>
#include <qcc/KeyBlob.h>

#include <Status.h>

// This status code is defined in ntstatus.h but including nststatus.h generate numerous compiler
// warnings about duplicate defines.
#ifndef STATUS_AUTH_TAG_MISMATCH
#define STATUS_AUTH_TAG_MISMATCH  ((NTSTATUS)0xC000A002L)
#endif

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

// Cache of open algorithm handles
static BCRYPT_ALG_HANDLE ecbHandle = 0;
static BCRYPT_ALG_HANDLE ccmHandle = 0;

class Crypto_AES::KeyState {
  public:

    KeyState(size_t len) : handle(0) {
        keyObj = new uint8_t[len];
    }

    ~KeyState() {
        // Must destroy the handle BEFORE freeing the key object
        if (handle) {
            BCryptDestroyKey(handle);
        }
        delete [] keyObj;
    }

    BCRYPT_KEY_HANDLE handle;
    uint8_t* keyObj;
};

Crypto_AES::Crypto_AES(const KeyBlob& key, Mode mode) : mode(mode), keyState(NULL)
{
    QStatus status;
    BCRYPT_ALG_HANDLE aesHandle = 0;

    // We depend on this being true
    assert(sizeof(Block) == 16);

    if (mode == CCM) {
        if (!ccmHandle) {
            if (BCryptOpenAlgorithmProvider(&ccmHandle, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0) < 0) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to open AES algorithm provider"));
                return;
            }
            // Enable CCM
            if (BCryptSetProperty(ccmHandle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CCM, wcslen(BCRYPT_CHAIN_MODE_CCM) + 1, 0) < 0) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to enable CCM mode on AES algorithm provider"));
                return;
            }
        }
        aesHandle = ccmHandle;
    } else {
        if (!ecbHandle) {
            if (BCryptOpenAlgorithmProvider(&ecbHandle, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0) < 0) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to open AES algorithm provider"));
                return;
            }
        }
        aesHandle = ecbHandle;
    }

    // Initialize a BCRYPT key blob with the key
    ULONG kbhLen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + key.GetSize();
    BCRYPT_KEY_DATA_BLOB_HEADER* kbh = (BCRYPT_KEY_DATA_BLOB_HEADER*)malloc(kbhLen);
    kbh->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    kbh->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    kbh->cbKeyData = key.GetSize();
    memcpy(kbh + 1, key.GetData(), key.GetSize());

    // Get length of key object and allocate the object
    DWORD got;
    ULONG keyObjLen;
    BCryptGetProperty(aesHandle, BCRYPT_OBJECT_LENGTH, (PBYTE)&keyObjLen, sizeof(ULONG), &got, 0);

    keyState = new KeyState(keyObjLen);

    if (BCryptImportKey(aesHandle, NULL, BCRYPT_KEY_DATA_BLOB, &keyState->handle, keyState->keyObj, keyObjLen, (PUCHAR)kbh, kbhLen, 0) < 0) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("Failed to import AES key"));
        delete keyState;
        keyState = NULL;
    }
    free(kbh);
}

Crypto_AES::~Crypto_AES()
{
    delete keyState;
}

QStatus Crypto_AES::Encrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check we are initialized for encryption
     */
    if (mode != ECB_ENCRYPT) {
        return ER_CRYPTO_ERROR;
    }
    ULONG len = numBlocks * sizeof(Block);
    ULONG clen;
    if (BCryptEncrypt(keyState->handle, (PUCHAR)in, len, NULL, NULL, 0, (PUCHAR)out, len, &clen, 0) < 0) {
        return ER_CRYPTO_ERROR;
    } else {
        return ER_OK;
    }
}

QStatus Crypto_AES::Encrypt(const void* in, size_t len, Block* out, uint32_t numBlocks)
{
    QStatus status;

    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check the lengths make sense
     */
    if (numBlocks != NumBlocks(len)) {
        return ER_CRYPTO_ERROR;
    }
    /*
     * Check for a partial final block
     */
    size_t partial = len % sizeof(Block);
    if (partial) {
        numBlocks--;
        status = Encrypt((Block*)in, out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            memcpy(&padBlock, ((const uint8_t*)in) + numBlocks * sizeof(Block), partial);
            status = Encrypt(&padBlock, out + numBlocks, 1);
        }
    } else {
        status = Encrypt((const Block*)in, out, numBlocks);
    }
    return status;
}

QStatus Crypto_AES::Decrypt(const Block* in, Block* out, uint32_t numBlocks)
{
    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check we are initialized for decryption
     */
    if (mode != ECB_DECRYPT) {
        return ER_CRYPTO_ERROR;
    }
    ULONG len = numBlocks * sizeof(Block);
    ULONG clen;
    if (BCryptDecrypt(keyState->handle, (PUCHAR)in, len, NULL, NULL, 0, (PUCHAR)out, len, &clen, 0) < 0) {
        return ER_CRYPTO_ERROR;
    } else {
        return ER_OK;
    }
}

QStatus Crypto_AES::Decrypt(const Block* in, uint32_t numBlocks, void* out, size_t len)
{
    QStatus status;

    if (!in || !out) {
        return in ? ER_BAD_ARG_1 : ER_BAD_ARG_2;
    }
    /*
     * Check the lengths make sense
     */
    if (numBlocks != NumBlocks(len)) {
        return ER_CRYPTO_ERROR;
    }
    /*
     * Check for a partial final block
     */
    size_t partial = len % sizeof(Block);
    if (partial) {
        numBlocks--;
        status = Decrypt(in, (Block*)out, numBlocks);
        if (status == ER_OK) {
            Block padBlock;
            status = Decrypt(in + numBlocks, &padBlock, 1);
            memcpy(((uint8_t*)out) + sizeof(Block) * numBlocks, &padBlock, partial);
        }
    } else {
        status = Decrypt(in, (Block*)out, numBlocks);
    }
    return status;
}

QStatus Crypto_AES::Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
{
    QStatus status = ER_OK;
    /*
     * Check we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if (!in && len) {
        return ER_BAD_ARG_1;
    }
    if (!out && len) {
        return ER_BAD_ARG_2;
    }
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_8;
    }
    // Address to put the mac
    uint8_t* mac = (uint8_t*)out + len;
    // Set CCM cipher mode info
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO cmi;
    BCRYPT_INIT_AUTH_MODE_INFO(cmi);

    // Zero pad nonce to a minimum size of 11 bytes.
    uint8_t npad[11];
    if (nLen < 11) {
        memcpy(npad, nonce.GetData(), nLen);
        memset(npad + nLen, 0, 11 - nLen);
        cmi.pbNonce = (PUCHAR)npad;
        nLen = 11;
    } else {
        cmi.pbNonce = (PUCHAR)nonce.GetData();
    }
    cmi.cbNonce = nLen;

    cmi.pbAuthData = (PUCHAR)addData;
    cmi.cbAuthData = addLen;
    cmi.pbTag = mac;
    cmi.cbTag = authLen;
    cmi.pbMacContext = NULL;
    cmi.cbMacContext = 0;
    cmi.cbAAD = 0;
    cmi.cbData = 0;
    cmi.dwFlags = 0;

    ULONG clen;
    Block iv;
    NTSTATUS ntstatus = BCryptEncrypt(keyState->handle, (PUCHAR)in, len, &cmi, NULL, 0, (PUCHAR)out, len, &clen, 0);
    if (ntstatus < 0) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("CCM mode encryption failed NTSTATUS=%x", ntstatus));
    } else {
        assert(len == clen);
        len += authLen;
    }
    return status;
}


QStatus Crypto_AES::Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen)
{
    QStatus status = ER_OK;
    /*
     * Check we are initialized for CCM
     */
    if (mode != CCM) {
        return ER_CRYPTO_ERROR;
    }
    size_t nLen = nonce.GetSize();
    if (!in && len) {
        return ER_BAD_ARG_1;
    }
    if (!out && len) {
        return ER_BAD_ARG_2;
    }
    if (nLen < 4 || nLen > 14) {
        return ER_BAD_ARG_4;
    }
    if ((authLen < 4) || (authLen > 16)) {
        return ER_BAD_ARG_8;
    }
    // Address to find the mac
    uint8_t* mac = (uint8_t*)in + len - authLen;
    // Set CCM cipher mode info
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO cmi;
    BCRYPT_INIT_AUTH_MODE_INFO(cmi);

    // Zero pad nonce to a minimum size of 11 bytes.
    uint8_t npad[11];
    if (nLen < 11) {
        memcpy(npad, nonce.GetData(), nLen);
        memset(npad + nLen, 0, 11 - nLen);
        cmi.pbNonce = (PUCHAR)npad;
        nLen = 11;
    } else {
        cmi.pbNonce = (PUCHAR)nonce.GetData();
    }
    cmi.cbNonce = nLen;

    cmi.pbAuthData = (PUCHAR)addData;
    cmi.cbAuthData = addLen;
    cmi.pbTag = mac;
    cmi.cbTag = authLen;
    cmi.pbMacContext = NULL;
    cmi.cbMacContext = 0;
    cmi.cbAAD = 0;
    cmi.cbData = 0;
    cmi.dwFlags = 0;

    ULONG clen;
    NTSTATUS ntstatus = BCryptDecrypt(keyState->handle, (PUCHAR)in, len - authLen, &cmi, NULL, 0, (PUCHAR)out, len, &clen, 0);
    if (ntstatus < 0) {
        if (ntstatus == STATUS_AUTH_TAG_MISMATCH) {
            status = ER_AUTH_FAIL;
        } else {
            status = ER_CRYPTO_ERROR;
        }
        QCC_LogError(status, ("CCM mode decryption failed NTSTATUS=%x", ntstatus));
    } else {
        len -= authLen;
        assert(len == clen);
    }
    return status;
}
