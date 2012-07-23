#ifndef _CRYPTO_H
#define _CRYPTO_H
/**
 * @file
 *
 * This file provide wrappers around cryptographic algorithms.
 */

/******************************************************************************
 * Copyright 2009-2012, Qualcomm Innovation Center, Inc.
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

#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include <qcc/KeyBlob.h>
#include <qcc/Stream.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>


namespace qcc {

/**
 * RSA public-key encryption and decryption class
 */
class Crypto_RSA {

  public:

    /**
     * Class for calling back to get a passphrase for encrypting or decrypting a private key.
     */
    class PassphraseListener {
      public:
        /**
         * Virtual destructor for derivable class.
         */
        virtual ~PassphraseListener() { }

        /**
         * Callback to request a passphrase.
         *
         * @param passphrase  Returns the passphrase.
         * @param toWrite     If true indicates the passphrase is being used to write a private key,
         *                    if false indicates the passphrase is for reading a private key.
         *
         * @return  Returns false if the passphrase request has been rejected.
         */
        virtual bool GetPassphrase(qcc::String& passphrase, bool toWrite) = 0;
    };

    /**
     * Constructor to generate a new key pair
     *
     * @param modLen    The length of the key in bits. This must be a multiple of 64 and >= 512
     */
    Crypto_RSA(uint32_t keyLen) : size(keyLen / 8), cert(NULL), key(NULL) { Generate(keyLen); }

    /**
     * Default constructor.
     */
    Crypto_RSA();

    /**
     * Import a private key from a PKCS#8 encoded string.
     *
     * @param pkcs8        The PKCS#8 encoded string
     * @param passphrase   The passphrase required to decode the private key
     * @return  - ER_OK if the key was successfully imported.
     *          - ER_AUTH_FAIL if the passphrase was incorrect or the PKCS string was invalid.
     *          - Other error status.
     */
    QStatus ImportPKCS8(const qcc::String& pkcs8, const qcc::String& passphrase);

    /**
     * Import a private key from a PKCS#8 encoded string.
     *
     * @param pkcs8      The PKCS#8 encoded string
     * @param listener   The listener to call if a passphrase is required to decrypt a private key.
     * @return  - ER_OK if the key was successfully imported.
     *          - ER_AUTH_FAIL if the passphrase was incorrect or the PKCS string was invalid.
     *          - ER_AUTH_REJECT if the listener rejected the authentication request.
     *          - Other error status.
     */
    QStatus ImportPKCS8(const qcc::String& pkcs8, PassphraseListener* listener);

    /**
     * Import a public key from a PEM-encoded public key.
     *
     * @param pem   The PEM encoded key.
     */
    QStatus ImportPEM(const qcc::String& pem);

    /**
     * Import a private key from an encrypted keyblob
     *
     * @param keyblob      The encrypted keyblob
     * @param passphrase   The passphrase required to decode the private key
     * @return  - ER_OK if the key was successfully imported.
     *          - ER_AUTH_FAIL if the passphrase was incorrect or the keyblob was invalid.
     *          - Other error status.
     */
    QStatus ImportPrivateKey(const qcc::KeyBlob& keyBlob, const qcc::String& passphrase);

    /**
     * Import a private key from an encrypted keyblob
     *
     * @param keyblob    The encrypted keyblob
     * @param listener   The listener to call if a passphrase is required to decrypt a private key.
     * @return  - ER_OK if the key was successfully imported.
     *          - ER_AUTH_FAIL if the passphrase was incorrect or the keyblob was invalid.
     *          - ER_AUTH_REJECT if the listener rejected the authentication request.
     *          - Other error status.
     */
    QStatus ImportPrivateKey(const qcc::KeyBlob& keyBlob, PassphraseListener* listener);

    /**
     * Returns a platform-specific encrypted key blob for a private key. It should be assumed that
     * the keyblob is not transportable off device since the content may be no more than a reference
     * to a key held in a internal platform key store.
     *
     * @param keyBlob      Returns the key blob with encrypted private key.
     * @param passphrase   The passphrase to encrypt the private key
     * @return  - ER_OK if the key was exported.
     *          - Other error status.
     */
    QStatus ExportPrivateKey(qcc::KeyBlob& keyBlob, const qcc::String& passphrase);

    /**
     * Returns a platform-specific encrypted key blob for a private key. It should be assumed that
     * the keyblob is not transportable off device since the content may be no more than a reference
     * to a key held in a internal platform key store.
     *
     * @param keyBlob      Returns the key blob with encrypted private key.
     * @param listener     The listener to call to obtain a passphrase to encrypt the private key.
     * @return  - ER_OK if the key was exported.
     *          - ER_AUTH_REJECT if the listener rejected the authentication request.
     *          - Other error status.
     */
    QStatus ExportPrivateKey(qcc::KeyBlob& keyBlob, PassphraseListener* listener);

    /**
     * Exports the PEM encoded string for a public key.
     *
     * @param pem   Returns the PEM encoded public key.
     * @return  - ER_OK if the key was encoded.
     *          - Other error status.
     */
    QStatus ExportPEM(qcc::String& pem);

    /**
     * Returns the RSA modulus size in bytes. The size can be used to determine the buffer size that
     * must must allocated to receive an encrypted value.
     *
     * @return The modulus size in bytes.
     */
    size_t GetSize();

    /**
     * Returns the maximum size of the message digest (or other data) that can be encrypted. This is
     * also the minimum size of the buffer that must be provided when decrypting.
     *
     * @return The maximum digest size in bytes.
     */
    size_t MaxDigestSize() { return (GetSize() - 12); }

    /**
     * Encrypt data using the public key.
     *
     * @param inData   The data to be encrypted
     * @param inLen    The length of the data to be encrypted
     * @param outData  The buffer to receive the encrypted data, must be <= GetSize()
     * @param outLen   On input the length of outData, this must be >= GetSize(), on return the
     *                 number of bytes on inData encrypted.
     *
     * @return  ER_OK if the data was encrypted.
     */
    QStatus PublicEncrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen);

    /**
     * Decrypt data using the private key.
     *
     * @param inData   The data to be decrypted
     * @param inLen    The length of the data to be decrypted
     * @param outData  The buffer to receive the decrypted data, must be <= GetSize()
     * @param outLen   On input the length of outData, this must be >= GetSize(), on return the
     *                 number of bytes on inData decrypted.
     *
     * @return  ER_OK if the data was encrypted.
     */
    QStatus PrivateDecrypt(const uint8_t* inData, size_t inLen, uint8_t* outData, size_t& outLen);

    /**
     * Generate a self-issued X509 certificate for this RSA key.
     *
     * @param name  The common name for this certificate.
     * @param app   The application that is requesting this certificate.
     */
    QStatus MakeSelfCertificate(const qcc::String& name, const qcc::String& app);

    /**
     * Use this private key to sign a message digest and return the digital signature.
     *
     * @param digest    The digest to sign.
     * @param digLen    The length of the digest to sign.
     * @param signature Returns the digital signature.
     * @param sigLen    On input the length of signature, this must be >= GetSize(), on return the
     *                  number of bytes in the signature.
     *
     * @return     - ER_OK if the signature was generated.
     *             - An error status indicating the error.
     */
    QStatus SignDigest(const uint8_t* digest, size_t digLen, uint8_t* signature, size_t& sigLen);

    /**
     * Use this private key to verify a message digest and return success or failure
     *
     * @param digest    The digest to sign.
     * @param digLen    The length of the digest to sign.
     * @param signature The digital signature to verify
     * @param sigLen    The length of the signature.
     *
     * @return     - ER_OK if the signature was verified.
     *             - ER_AUTH_FAIL if the verification was unsuccessful
     *             - An error status indicating the error.
     */
    QStatus VerifyDigest(const uint8_t* digest, size_t digLen, const uint8_t* signature, size_t sigLen);

    /**
     * Use this private key to sign a document and return the digital signature.
     *
     * @param doc       The document to sign.
     * @param docLen    The length of the document to sign.
     * @param signature Returns the digital signature.
     * @param sigLen    On input the length of signature, this must be >= GetSize(), on return the
     *                  number of bytes in the signature.
     *
     * @return     - ER_OK if the signature was generated.
     *             - An error status indicating the error.
     */
    QStatus Sign(const uint8_t* data, size_t len, uint8_t* signature, size_t& sigLen);

    /**
     * Use this public key to verify the digital signature on a document.
     *
     * @param doc       The document to sign.
     * @param docLen    The length of the document to sign.
     * @param signature Returns the digital signature.
     * @param sigLen    On input the length of signature, this must be >= GetSize(), on return the
     *                  number of bytes in the signature.
     *
     * @return     - ER_OK if the signature was verified.
     *             - ER_AUTH_FAIL if the verfication failed.
     *             - Other status codes indicating an error.
     */
    QStatus Verify(const uint8_t* data, size_t len, const uint8_t* signature, size_t sigLen);

    /**
     * Returns a human readable string for a cert if there is one associated with this key.
     *
     * @return A string for the cert or and empty string if there is no cert.
     */
    qcc::String CertToString();

    /**
     * Destructor
     */
    ~Crypto_RSA();

  private:

    /**
     * Generate a public/private key pair
     */
    void Generate(uint32_t keyLen);

    /**
     * Assignment operator is private
     */
    Crypto_RSA& operator=(const Crypto_RSA& other);

    /**
     * Copy constructor is private
     */
    Crypto_RSA(const Crypto_RSA& other);

    size_t size;
    void* cert;
    void* key;
};

/**
 * AES block encryption/decryption class
 */
class Crypto_AES  {

  public:

    /**
     * Size of an AES128 key in bytes
     */
    static const size_t AES128_SIZE = (128 / 8);

    /**
     * AES modes
     */
    typedef enum {
        ECB_ENCRYPT, ///< Flag to constructor indicating key is being used for ECB encryption
        ECB_DECRYPT, ///< Flag to constructor indicating key is being used for ECB decryption
        CCM          ///< Flag to constructor indicating key is being used CCM mode
    } Mode;

    /**
     * CryptoAES constructor.
     *
     * @param key   The AES key
     * @param mode  Specifies the operation mode.
     */
    Crypto_AES(const KeyBlob& key, Mode mode);

    /**
     * Data is encrypted or decrypted in 16 byte blocks.
     *
     * Note we depend on sizeof(Block) == 16
     */
    class Block {
      public:
        uint8_t data[16];
        /**
         * Constructor that initializes the block.
         *
         * @param ival  The initial value.
         */
        Block(uint8_t ival) { memset(data, ival, sizeof(data)); }
        /**
         * Default constructor.
         */
        Block() { };
        /**
         * Null pad end of block
         */
        void Pad(size_t padLen) {
            assert(padLen <= 16);
            if (padLen > 0) { memset(&data[16 - padLen], 0, padLen); }
        }
    };

    /**
     * Helper function to return the number of blocks required to hold a encrypt a number of bytes.
     *
     * @param len  The data length
     * @return     The number of blocks for data of the requested length.
     */
    static size_t NumBlocks(size_t len) { return (len + sizeof(Block) - 1) / sizeof(Block); }

    /**
     * Encrypt some data blocks.
     *
     * @param in          An array of data blocks to encrypt
     * @param out         The encrypted data blocks
     * @param numBlocks   The number of blocks to encrypt.
     *
     * @return ER_OK if the data was encrypted.
     */
    QStatus Encrypt(const Block* in, Block* out, uint32_t numBlocks);

    /**
     * Encrypt some data. The encrypted data is padded as needed to round up to a whole number of blocks.
     *
     * @param in          An array of data blocks to encrypt
     * @param len         The length of the input data
     * @param out         The encrypted data blocks
     * @param numBlocks   The number of blocks for the encrypted data, compute this using NumEncryptedBlocks(len)
     * @return ER_OK if the data was encrypted.
     */
    QStatus Encrypt(const void* in, size_t len, Block* out, uint32_t numBlocks);

    /**
     * Decrypt some data blocks.
     *
     * @param in          An array of data blocks to decrypt
     * @param out         The decrypted data blocks
     * @param numBlocks   The number of blocks to decrypt.
     * @return ER_OK if the data was decrypted.
     */
    QStatus Decrypt(const Block* in, Block* out, uint32_t numBlocks);

    /**
     * Decrypt some data. The decrypted data is truncated at len
     *
     * @param in          An array of data blocks to decrypt.
     * @param len         The number of blocks of input data.
     * @param out         The decrypted data.
     * @param numBlocks   The number of blocks for the decrypted data, compute this using NumEncryptedBlocks(len)
     *
     * @return ER_OK if the data was encrypted.
     */
    QStatus Decrypt(const Block* in, uint32_t numBlocks, void* out, size_t len);

    /**
     * Encrypt some data using CCM mode.
     *
     * @param in          Pointer to the data to encrypt
     * @param out         The encrypted data, this can be the same as in. The size of this buffer
     *                    must be large enough to hold the encrypted input data and the
     *                    authentication field. This means at least (len + authLen) bytes.
     * @param len         On input the length of the input data,returns the length of the output data.
     * @param nonce       A nonce with length between 4 and 14 bytes. The nonce must contain a variable
     *                    component that is different for every encryption in a given session.
     * @param addData     Additional data to be authenticated.
     * @param addLen      Length of the additional data.
     * @param authLen     Lengh of the authentication field, must be in range 4..16
     *
     * @return ER_OK if the data was encrypted.
     */
    QStatus Encrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen = 8);

    /**
     * Convenience wrapper for encrypting and authenticating a header and message in-place.
     *
     * @param msg      Pointer to the entire message. The message buffer must be long enough to
     *                 allow for the authentication field (of length authLen) to be appended.
     * @param msgLen   On input, the length in bytes of the plaintext message, on output the expanded
     *                 length of the encrypted message.
     * @param hdrLen   Length in bytes of the header portion of the message
     * @param nonce    A nonce with length between 4 and 14 bytes. The nonce must contain a variable
     *                 component that is different for every encryption in a given session.
     * @param authLen  Lengh of the authentication field, must be in range 4..16
     */
    QStatus Encrypt_CCM(void* msg, size_t& msgLen, size_t hdrLen, const KeyBlob& nonce, uint8_t authLen = 8)
    {
        if (!msg) {
            return ER_BAD_ARG_1;
        }
        if (msgLen < hdrLen) {
            return ER_BAD_ARG_2;
        }
        size_t len = msgLen - hdrLen;
        QStatus status = Encrypt_CCM((uint8_t*)msg + hdrLen, (uint8_t*)msg + hdrLen, len, nonce, msg, hdrLen, authLen);
        msgLen = hdrLen + len;
        return status;
    }

    /**
     * Decrypt some data using CCM mode.
     *
     * @param in          An array of to encrypt
     * @param out         The encrypted data blocks, this can be the same as in.
     * @param len         On input the length of the input data, returns the length of the output data.
     * @param nonce       A nonce with length between 11 and 14 bytes. The nonce must contain a variable
     *                    component that is different for every encryption in a given session.
     * @param addData     Additional data to be authenticated.
     * @param addLen      Length of the additional data.
     * @param authLen     Length of the authentication field, must be in range 4..16
     *
     * @return ER_OK if the data was decrypted and verified.
     *         ER_AUTH_FAIL if the decryption failed.
     */
    QStatus Decrypt_CCM(const void* in, void* out, size_t& len, const KeyBlob& nonce, const void* addData, size_t addLen, uint8_t authLen = 8);

    /**
     * Convenience wrapper for decrypting and authenticating a header and message in-place.
     *
     * @param msg      Pointer to the entire message.
     * @param msgLen   On input, the length in bytes of the encrypted message, on output the
     *                 length of the plaintext message.
     * @param hdrLen   Length in bytes of the header portion of the message
     * @param nonce    A nonce with length between 11 and 14 bytes. The nonce must contain a variable
     *                 component that is different for every encryption in a given session.
     * @param authLen  Lengh of the authentication field, must be in range 4..16
     */
    QStatus Decrypt_CCM(void* msg, size_t& msgLen, size_t hdrLen, const KeyBlob& nonce, uint8_t authLen = 8)
    {
        if (!msg) {
            return ER_BAD_ARG_1;
        }
        if (msgLen < hdrLen) {
            return ER_BAD_ARG_2;
        }
        size_t len = msgLen - hdrLen;
        QStatus status = Decrypt_CCM((uint8_t*)msg + hdrLen, (uint8_t*)msg + hdrLen, len, nonce, msg, hdrLen, authLen);
        msgLen = hdrLen + len;
        return status;
    }

    /**
     * Destructor
     */
    ~Crypto_AES();

  private:

    Crypto_AES() { }

    /**
     * Copy constructor is private
     */
    Crypto_AES(const Crypto_AES& other);

    /**
     * Assigment operator is private
     */
    Crypto_AES& operator=(const Crypto_AES& other);

    /**
     * Flag indicating the mode for the class instance.
     */
    Mode mode;

    /**
     * Opaque type for the internal state
     */
    struct KeyState;

    /**
     * Private internal key state
     */
    KeyState* keyState;

};

/**
 * Generic hash calculation interface abstraction class.
 */
class Crypto_Hash {
  public:

    /**
     * Default constructor
     */
    Crypto_Hash() : MAC(false), initialized(false), ctx(NULL) { }

    /**
     * The destructor cleans up the context.
     */
    virtual ~Crypto_Hash();

    /**
     * Virtual initializer to be implemented by derivative classes.  The
     * derivative classes should call the protected Crypto_Hash::Init()
     * function with the appropriate algorithm.
     *
     * @param hmacKey   [optional] An array containing the HMAC key.
     *                             (Defaults to NULL for no key.)
     * @param keyLen    [optional] The size of the HMAC key.
     *                             (Defaults to 0 for no key.)
     *
     * @return  Indication of success or failure.
     */
    virtual QStatus Init(const uint8_t* hmacKey = NULL, size_t keyLen = 0) = 0;

    /**
     * Update the digest with the contents of the specified buffer.
     *
     * @param buf       Buffer with data for feeding to the hash algorithm.
     * @param bufSize   Size of the buffer.
     *
     * @return  Indication of success or failure.
     */
    QStatus Update(const uint8_t* buf, size_t bufSize);

    /**
     * Update the digest with the contents of a string
     *
     * @param str   The string to hash.
     *
     * @return  Indication of success or failure.
     */
    QStatus Update(const qcc::String& str);

    /**
     * Retrieve the digest into the supplied buffer.  It is assumed that buffer is large enough to
     * store the digest. Unless keepAlive is true, after the digest has been computed the hash
     * instance is not longer usable until re-initialized. Keep alive is not allowed for HMAC.
     *
     * @param digest     Buffer for storing the digest.
     * @param keepAlive  If true, keep the hash computation alive so additional data can be added
     *                   and the updated digest can be computed.
     *
     * @return  Indication of success or failure.
     */
    QStatus GetDigest(uint8_t* digest, bool keepAlive = false);

  protected:

    /// Typedef for abstracting the hash algorithm specifier.
    typedef enum {
        SHA1,    ///< SHA1 algorithm specifier
        MD5,     ///< MD5 algorithm specifier
        SHA256   ///< SHA256 algorithm specifier
    } Algorithm;

    static const size_t SHA1_SIZE = 20;   ///< SHA1 digest size - 20 bytes == 160 bits
    static const size_t MD5_SIZE = 16;    ///< MD5 digest size - 16 bytes == 128 bits
    static const size_t SHA256_SIZE = 32; ///< SHA256 digest size - 32 bytes == 256 bits

    /**
     * The common initializer.  Derivative classes should call this from their
     * public implementation of Init().
     *
     * @param alg       Algorithm to be used for the hash function.
     * @param hmacKey   An array containing the HMAC key.
     * @param keyLen    The size of the HMAC key.
     *
     * @return  Indication of success or failure.
     */
    QStatus Init(Algorithm alg, const uint8_t* hmacKey = NULL, size_t keyLen = 0);

  private:

    /**
     * No copy constructor
     */
    Crypto_Hash(const Crypto_Hash& other);

    /**
     * No assigment operator
     */
    Crypto_Hash& operator=(const Crypto_Hash& other);

    bool MAC;          ///< Flag indicating if computing a MAC
    bool initialized;  ///< Flag indicating hash has been initialized

    size_t digestSize;  ///< Digest size
    struct Context;     ///< Opaque context type

    Context* ctx;       ///< Pointer to context.
};

/**
 * SHA1 hash calculation interface abstraction class.
 */
class Crypto_SHA1 : public Crypto_Hash {
  public:
    virtual QStatus Init(const uint8_t* hmacKey = NULL, size_t keyLen = 0)
    {
        return Crypto_Hash::Init(SHA1, hmacKey, keyLen);
    }

    static const size_t DIGEST_SIZE = Crypto_Hash::SHA1_SIZE;
};

/**
 * MD5 hash calculation interface abstraction class.
 */
class Crypto_MD5 : public Crypto_Hash {
  public:
    virtual QStatus Init(const uint8_t* hmacKey = NULL, size_t keyLen = 0)
    {
        return Crypto_Hash::Init(MD5, hmacKey, keyLen);
    }

    static const size_t DIGEST_SIZE = Crypto_Hash::MD5_SIZE;
};

/**
 * SHA256 hash calculation interface abstraction class.
 */
class Crypto_SHA256 : public Crypto_Hash {
  public:
    virtual QStatus Init(const uint8_t* hmacKey = NULL, size_t keyLen = 0)
    {
        return Crypto_Hash::Init(SHA256, hmacKey, keyLen);
    }

    static const size_t DIGEST_SIZE = Crypto_Hash::SHA256_SIZE;
};

/**
 * This function uses one of more HMAC hashes to implement the PRF (Pseudorandom Function) defined
 * in RFC 5246 and is used to expand a secret into an arbitrarily long block of data from which keys
 * can be derived. Per the recommendation in RFC 5246 this function uses the SHA256 hash function.
 *
 * @param secret  A keyblob containing the secret being expanded.
 * @param label   An ASCII string that is hashed in with the other data.
 */
QStatus Crypto_PseudorandomFunction(const KeyBlob& secret, const char* label, const qcc::String& seed, uint8_t* out, size_t outLen);


/**
 *  Secure Remote Password (SRP6) class. This implements the core algorithm for Secure Remote
 *  Password authentication protocol as defined in RFC 5054.
 */
class Crypto_SRP {
  public:

    /**
     * Initialize the client side of SRP. The client is initialized with a string from the server
     * that encodes the startup parameters.
     *
     * @param fromServer   Hex-encoded parameter string from the server.
     * @param toServer     Retusn a hex-encode parameter string to be sent to the server.
     *
     * @return  - ER_OK if the initialization was succesful.
     *          - ER_BAD_STRING_ENCODING if the hex-encoded parameter block is badly formed.
     *          - ER_CRYPTO_INSUFFICIENT_SECURITY if the parameters are not acceptable.
     *          - ER_CRYPTO_ILLEGAL_PARAMETERS if any parameters have illegal values.
     */
    QStatus ClientInit(const qcc::String& fromServer, qcc::String& toServer);

    /**
     * Final phase of the client side .
     *
     * @param id   The user use id to authenticate.
     * @param pwd  The password corresponding to the user id.
     *
     * @return ER_OK or an error status.
     */
    QStatus ClientFinish(const qcc::String& id, const qcc::String& pwd);

    /**
     * Initialize the server side of SRP. The server is initialized with the user id and password
     * and returns a hex-encoded string to be sent to the client that encodes the startup parameters.
     *
     * @param id        The id of the user being authenticated.
     * @param pwd       The password corresponding to the user id.
     * @param toClient  Hex-encoded parameter string to be sent to the client.
     *
     * @return  - ER_OK or an error status.
     */
    QStatus ServerInit(const qcc::String& id, const qcc::String& pwd, qcc::String& toClient);

    /**
     * Initialize the server side of SRP using a hex-encoded verifier string that encodes the user name and
     * password. Returns a hex-encoded string to be sent to the client that encodes the startup parameters.
     *
     * @param verifier   An encoding of the user id and password that is used to verify the client.
     * @param toClient   Hex-encoded parameter string to be sent to the client.
     *
     * @return  - ER_OK or an error status.
     */
    QStatus ServerInit(const qcc::String& verifier, qcc::String& toClient);

    /**
     * Final phase of the server side of SRP. The server is provided with a string from the client
     * that encodes the client's parameters.
     *
     * @param fromClient   Hex-encoded parameter string from the client.
     *
     * @return  - ER_OK if the initialization was succesful.
     *          - ER_BAD_STRING_ENCODING if the hex-encoded parameter block is badly formed.
     *          - ER_CRYPTO_ILLEGAL_PARAMETERS if any parameters have illegal values.
     */
    QStatus ServerFinish(const qcc::String fromClient);

    /**
     * Get encoded verifier. The verifier can stored for future use by the server-side of the
     * protocol without requiring knowledge of the password. This function can be called immediately
     * after ServerInit() has been called.
     *
     * @return Returns the verifier string.
     */
    qcc::String ServerGetVerifier();

    /**
     * Get the computed premaster secret. This function should be called after ServerFinish() or
     * ClientFinish() to obtain the shared premaster secret.
     *
     * @param premaster  Returns a key blob initialized with the premaster secret.
     */
    void GetPremasterSecret(KeyBlob& premaster);

    /**
     * Constructor
     */
    Crypto_SRP();

    /**
     * Destructor
     */
    ~Crypto_SRP();

    /**
     * Test interface runs the RFC 5246 test vector.
     */
    QStatus TestVector();

  private:

    /**
     * No copy constructor
     */
    Crypto_SRP(const Crypto_SRP& other);

    /**
     * No assigment operator
     */
    Crypto_SRP& operator=(const Crypto_SRP& other);

    void ServerCommon(qcc::String& toClient);

    class BN;
    BN* bn;

};

/**
 *  ASN.1 encoding and decoding class. This implements encoding and decoding algorithms for
 *  DER-formatted ASN.1 string and related helper functions.
 */
class Crypto_ASN1 {

  public:

    /**
     * Decode a DER formatted ASN1 data blob returning the decoded values as a variable length list
     * of argument. The expected structure of the ASN1 data blob is described by the syntax
     * parameter. The variable argument list must conform to the argument types for each syntactic
     * element as defined below.
     *
     * 'i'  ASN_INTEGER   An integer of 4 bytes or less the argument must be a pointer to a uint32
     *
     * 'l'  ASN_INTEGER   An arbitrary length integer, the argument must be a pointer to a qcc::String
     *
     * 'o'  ASN_OID       An ASN encoded object id, the argument must be a pointer to a qcc::String
     *
     * 'x'  ASN_OCTET     An octet string, the argument must be a pointer to a qcc::String
     *
     * 'b'  ASN_BITS      A bit string, the argument must be a pointer to a qcc::String followe by a
     *                    pointer to a size_t value to receive the bit length.
     *
     * 'n'  ASN_NULL      Null, there is no argument for this item
     *
     * 'u'  ASN_UTF8      A utf8 string, the argument must be a pointer to a qcc::String
     *
     * 'a'  ASN_ASCII     A printable string, the argument must be a pointer to a qcc::String
     *
     * 'p'  ASN_PRINTABLE A printable string, the argument must be a pointer to a qcc::String
     *
     * 't'  ASN_UTC_TIME  A UTC time string, the argument must be a pointer to a qcc::String
     *
     * '('  ASN_SEQ       Indicates the start of a sequence, there are no arguments this item.
     *
     * ')'                Indicates the end of a sequence, there are no arguments this item.
     *
     * '{'  ASN_SET_OF    Indicates the start of a set-of, there are no arguments this item.
     *
     * '}'                Indicates the end of a set-of, there are no arguments this item.
     *
     * '?'                A single element that is extracted but not decoded, the argument must be a
     *                    pointer to a qcc::String or NULL to ignore this item.
     *
     * '*'                Zero or more optional elements up to the end of the enclosing sequence or set
     *                    are skipped.
     *
     * '/'                Before the final syntatic element of a sequence or set this indicates that
     *                    the element is optional. If the element exists it is decoded. The argument
     *                    must be a pointer to a qcc::String and type of the following element must
     *                    be appropriate for this argument type. Note that is it not possible to
     *                    distinguish between a missing element and a zero length element.
     *
     * @param asn      The input data for the encoding
     * @param asnLen   The length of the input data
     * @param syntax   The structure to use for the decoding operation.
     * @param ...      The output arguments as required by the syntax parameter to receive the
     *                 decode values.
     *
     * @return ER_OK if the decode succeeded.
     *         An error status otherwise.
     *
     */
    static QStatus Decode(const uint8_t* asn, size_t asnLen, const char* syntax, ...)
    {
        if (!syntax) {
            return ER_BAD_ARG_1;
        }
        if (!asn) {
            return ER_BAD_ARG_2;
        }
        va_list argp;
        va_start(argp, syntax);
        QStatus status = DecodeV(syntax, asn, asnLen, &argp);
        va_end(argp);
        return status;
    }

    /**
     * Variation on Decode method that takes a qcc::String argument.
     *
     * @param asn      The input string for the encoding
     * @param syntax   The structure to use for the decoding operation.
     * @param ...      The output arguments as required by the syntax parameter to receive the
     *                 decode values.
     *
     * @return ER_OK if the decode succeeded.
     *         An error status otherwise.
     *
     */
    static QStatus Decode(const qcc::String& asn, const char* syntax, ...)
    {
        if (!syntax) {
            return ER_BAD_ARG_1;
        }
        va_list argp;
        va_start(argp, syntax);
        QStatus status = DecodeV(syntax, (const uint8_t*)asn.data(), asn.size(), &argp);
        va_end(argp);
        return status;
    }

    /**
     * Encode a variable length list of arguments into an ASN1 data blob. The structure of the ASN1
     * data blob is described by the syntax parameter. The variable argument list must conform to
     * the argument types for each syntactic element as defined below.
     *
     * 'i'  ASN_INTEGER   An integer of 4 bytes or less the argument must be a uint32
     *
     * 'l'  ASN_INTEGER   An arbitrary length integer, the argument must be a pointer to a qcc::String
     *
     * 'o'  ASN_OID       An ASN encoded object id, the argument must be a pointer to a qcc::String
     *
     * 'x'  ASN_OCTET     An octet string, the argument must be a pointer to a qcc::String
     *
     * 'b'  ASN_BITS      A bit string, the argument must be a pointer to a qcc::String followed by a
     *                    size_t value that specifies the bit length.
     *
     * 'n'  ASN_NULL      Null, there is no argument for this item
     *
     * 'u'  ASN_UTF8      A utf8 string, the argument must be a pointer to a qcc::String
     *
     * 'a'  ASN_ASCII     A printable string, the argument must be a pointer to a qcc::String
     *
     * 'p'  ASN_PRINTABLE A printable string, the argument must be a pointer to a qcc::String
     *
     * 't'  ASN_UTC_TIME  A UTC time string, the argument must be a pointer to a qcc::String
     *
     * '('  ASN_SEQ       Indicates the start of a sequence, there are no arguments this item.
     *
     * ')'                Indicates the end of a sequence, there are no arguments this item.
     *
     * '{'  ASN_SET_OF    Indicates the start of a set-of, there are no arguments this item.
     *
     * '}'                Indicates the end of a set-of, there are no arguments this item.
     *
     *
     * @param asn      The output string for the encoding
     * @param syntax   The structure to use for the encoding operation.
     * @param ...      The input arguments as required by the syntax parameter
     *
     * @return ER_OK if the encode succeeded.
     *         An error status otherwise.
     */
    static QStatus Encode(qcc::String& asn, const char* syntax, ...)
    {
        if (!syntax) {
            return ER_FAIL;
        }
        va_list argp;
        va_start(argp, syntax);
        QStatus status = EncodeV(syntax, asn, &argp);
        va_end(argp);
        return status;
    }

    /**
     * Decode a PEM base-64 ANSI string to binary.
     *
     * @param b64  The base-64 string to decode.
     * @param bin  The binary output of the decoding.
     *
     * @return ER_OK if the decode succeeded.
     *         An error status otherwise.
     */
    static QStatus DecodeBase64(const qcc::String& b64, qcc::String& bin);

    /**
     * Encode a binary string as a PEM base-64 ANSI string.
     *
     * @param bin  The binary string to encode.
     * @param b64  The base-64 output of the decoding.
     *
     * @return ER_OK if the encode succeeded.
     *         An error status otherwise.
     */
    static QStatus EncodeBase64(const qcc::String& bin, qcc::String& b64);

    /*
     * Render ASN.1 as a "human" readable string
     */
    static qcc::String ToString(const uint8_t* asn, size_t len, size_t indent = 0);

  private:

    static const uint8_t ASN_BOOLEAN   = 0x01;
    static const uint8_t ASN_INTEGER   = 0x02;
    static const uint8_t ASN_BITS      = 0x03;
    static const uint8_t ASN_OCTETS    = 0x04;
    static const uint8_t ASN_NULL      = 0x05;
    static const uint8_t ASN_OID       = 0x06;
    static const uint8_t ASN_UTF8      = 0x0C;
    static const uint8_t ASN_SEQ       = 0x10;
    static const uint8_t ASN_SET_OF    = 0x11;
    static const uint8_t ASN_PRINTABLE = 0x13;
    static const uint8_t ASN_ASCII     = 0x16;
    static const uint8_t ASN_UTC_TIME  = 0x17;

    static QStatus DecodeV(const char*& syntax, const uint8_t* asn, size_t asnLen, va_list* argpIn);

    static QStatus EncodeV(const char*& syntax, qcc::String& asn, va_list* argpIn);

    static qcc::String DecodeOID(const uint8_t* p, size_t len);

    static QStatus EncodeOID(qcc::String& asn, const qcc::String& oid);

    static bool DecodeLen(const uint8_t*& p, const uint8_t* eod, size_t& l);

    static void EncodeLen(qcc::String& asn, size_t l);
};

/**
 * Call platform specific API to get cryptographically random data.
 *
 * @param data  A buffer to receive the pseuod random number data.
 * @param len   The length of the buffer.
 *
 * @return ER_OK if a random number was succesfully generated.
 */
QStatus Crypto_GetRandomBytes(uint8_t* data, size_t len);

/**
 * Some crypto libraries (e.g. openssl) are not thread safe so the wrappers must obtain a mutual
 * exclusion lock before making calls into the crypto library. To obtain the lock declare an
 * instance of this class before calling any crypto library APIs
 */
class Crypto_ScopedLock {
  public:
    Crypto_ScopedLock();
    ~Crypto_ScopedLock();
};


}

#endif
