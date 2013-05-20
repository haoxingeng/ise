/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// ise_cipher.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EXT_UTILS_CIPHER_H_
#define _ISE_EXT_UTILS_CIPHER_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_global_defs.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_exceptions.h"

#include <string>
#include <memory>

namespace ise
{

namespace utils
{

///////////////////////////////////////////////////////////////////////////////
// class declares

class Format;
class Format_Copy;
class Format_HEX;
class Format_HEXL;
class Format_MIME32;
class Format_MIME64;

class Hash;
class HashBaseMD4;
class Hash_MD4;
class Hash_MD5;
class Hash_SHA;
class Hash_SHA1;

class Cipher;
class Cipher_Null;
class Cipher_Blowfish;
class Cipher_IDEA;
class Cipher_DES;
class Cipher_Gost;

///////////////////////////////////////////////////////////////////////////////
// Type Definitions

typedef string binary;

// The format type
enum FORMAT_TYPE
{
    FT_COPY,
    FT_HEX,
    FT_HEXL,
    FT_MIME32,
    FT_MIME64,
};

// Hash type
enum HASH_TYPE
{
    HT_MD4,
    HT_MD5,
    HT_SHA,
    HT_SHA1,
};

// Cipher type
enum CIPHER_TYPE
{
    CT_NULL,
    CT_BLOWFISH,
    CT_IDEA,
    CT_DES,
    CT_GOST,
};

// Cipher kind
enum CIPHER_KIND
{
    CK_ENCODE,
    CK_DECODE,
};

/*
  Cipher state represents the internal state of processing.
  CS_NEW         = cipher isn't initialized, .Init() must be called before en/decode
  CS_INITIALIZED = cipher is initialized by .Init(), eg. Keysetup was processed
  CS_ENCODE      = Encodeing was started, and more chunks can be encoded, but not decoded
  CS_DECODE      = Decodeing was started, and more chunks can be decoded, but not encoded
  CS_PADDED      = trough En/Decodeing the messagechunks are padded, no more chunks can
                   be processed, the cipher is blocked.
  CS_DONE        = Processing is finished and Cipher.Done was called. Now new En/Decoding
                   can be started without calling .Init() before. csDone is basicaly
                   identical to csInitialized, except Cipher.Buffer holds the encrypted
                   last state of Cipher.Feedback, thus Cipher.Buffer can be used as C-MAC.
*/
enum CIPHER_STATE
{
    CS_NEW         = 0x01,
    CS_INITIALIZED = 0x02,
    CS_ENCODE      = 0x04,
    CS_DECODE      = 0x08,
    CS_PADDED      = 0x10,
    CS_DONE        = 0x20,
};

/*
  The cipher modes:
  CM_CTSx = double CBC, with CFS8 padding of truncated final block
  CM_CBCx = Cipher Block Chainung, with CFB8 padding of truncated final block
  CM_CFB8 = 8bit Cipher Feedback mode
  CM_CFBx = CFB on Blocksize of Cipher
  CM_OFB8 = 8bit Output Feedback mode
  CM_OFBx = OFB on Blocksize bytes
  CM_CFS8 = 8Bit CFS, double CFB
  CM_CFSx = CFS on Blocksize bytes
  CM_ECBx = Electronic Code Book

  Modes CM_CBCx, CM_CTSx, CM_CFBx, CM_OFBx, CM_CFSx, CM_ECBx working on Blocks of
  Cipher.BufferSize bytes, on Blockcipher that's equal to Cipher.BlockSize.

  Modes CM_CFB8, CM_OFB8, CM_CFS8 work on 8 bit Feedback Shift Registers.

  Modes CM_CTSx, CM_CFSx, CM_CFS8 are prohibitary modes developed by me. These modes
  works such as CM_CBCx, CM_CFBx, CM_CFB8 but with double XOR'ing of the inputstream
  into Feedback register.

  Mode CM_ECBx need message padding to a multiple of Cipher.BlockSize and should
  be only used in 1byte Streamciphers.

  Modes CM_CTSx, CM_CBCx need no external padding, because internal the last truncated
  block is padded by CM_CFS8 or CM_CFB8. After padding these Mode can't be used to
  process more data. If it needed to process chunks of data then each chunk must
  be algined to Cipher.BufferSize bytes.

  Modes CM_CFBx,CM_CFB8,CM_OFBx,CM_OFB8,CM_CFSx,CM_CFS8 need no padding.
*/
enum CIPHER_MODE
{
    CM_CTSx,
    CM_CBCx,
    CM_CFB8,
    CM_CFBx,
    CM_OFB8,
    CM_OFBx,
    CM_CFS8,
    CM_CFSx,
    CM_ECBx,
};

// Crc32 type
enum CRC32_TYPE
{
    CRC32_STANDARD,    // PKZIP, PHP...
    CRC32_SPECIAL,
};

// Cipher context
struct CipherContext
{
    int keySize;               // maximal key size in bytes
    int blockSize;             // mininmal block size in bytes, eg. 1 = Streamcipher
    int bufferSize;            // internal buffersize in bytes
    int userSize;              // internal size in bytes of cipher dependend structures
    bool userSave;
};

///////////////////////////////////////////////////////////////////////////////
// Const Definitions

const CIPHER_MODE DEFAULT_CIPHER_MODE = CM_CTSx;

///////////////////////////////////////////////////////////////////////////////
// Misc Routines

binary hashString(HASH_TYPE hashType, const binary& sourceStr, PVOID digest = NULL);
binary hashBuffer(HASH_TYPE hashType, PVOID buffer, int dataSize, PVOID digest = NULL);
binary hashStream(HASH_TYPE hashType, Stream& stream, PVOID digest = NULL);
binary hashFile(HASH_TYPE hashType, char *fileName, PVOID digest = NULL);

binary md5(const binary& sourceStr);
binary md5(PVOID buffer, int dataSize);
binary md5(Stream& stream);
binary md5(char *fileName);

DWORD calcCrc32(PVOID data, int dataSize, DWORD lastResult = 0xFFFFFFFF, CRC32_TYPE crc32Type = CRC32_SPECIAL);
BYTE calcCrc8(PVOID data, int dataSize);

void encryptBuffer(CIPHER_TYPE cipherType, PVOID source, PVOID dest, int dataSize, char *key);
void decryptBuffer(CIPHER_TYPE cipherType, PVOID source, PVOID dest, int dataSize, char *key);
void encryptStream(CIPHER_TYPE cipherType, Stream& srcStream, Stream& destStream, char *key);
void decryptStream(CIPHER_TYPE cipherType, Stream& srcStream, Stream& destStream, char *key);
void encryptFile(CIPHER_TYPE cipherType, char *srcFileName, char *destFileName, char *key);
void decryptFile(CIPHER_TYPE cipherType, char *srcFileName, char *destFileName, char *key);

binary base64Encode(PVOID data, int dataSize);
binary base64Decode(PVOID data, int dataSize);
binary base16Encode(PVOID data, int dataSize);
binary base16Decode(PVOID data, int dataSize);

///////////////////////////////////////////////////////////////////////////////
// DataAlgoProgress

class DataAlgoProgress
{
public:
    virtual ~DataAlgoProgress() {}
    virtual void progress(INT64 min, INT64 max, INT64 pos) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Format

class Format
{
public:
    virtual ~Format() {}
    binary encode(const binary& value);
    binary encode(PVOID data, int size);
    binary decode(const binary& value);
    binary decode(PVOID data, int size);
    bool isValid(const binary& value);
    bool isValid(PVOID data, int size);
protected:
    virtual binary doEncode(PVOID data, int size) = 0;
    virtual binary doDecode(PVOID data, int size) = 0;
    virtual bool doIsValid(PVOID data, int size) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Format_Copy

class Format_Copy : public Format
{
protected:
    virtual binary doEncode(PVOID data, int size);
    virtual binary doDecode(PVOID data, int size);
    virtual bool doIsValid(PVOID data, int size);
};

///////////////////////////////////////////////////////////////////////////////
// Format_HEX

class Format_HEX : public Format
{
protected:
    virtual binary doEncode(PVOID data, int size);
    virtual binary doDecode(PVOID data, int size);
    virtual bool doIsValid(PVOID data, int size);
protected:
    virtual const char* charTable();
};

///////////////////////////////////////////////////////////////////////////////
// Format_HEXL

class Format_HEXL : public Format_HEX
{
protected:
    virtual const char* charTable();
};

///////////////////////////////////////////////////////////////////////////////
// Format_MIME32

class Format_MIME32 : public Format_HEX
{
protected:
    virtual binary doEncode(PVOID data, int size);
    virtual binary doDecode(PVOID data, int size);
protected:
    virtual const char* charTable();
};

///////////////////////////////////////////////////////////////////////////////
// Format_MIME64

class Format_MIME64 : public Format_HEX
{
protected:
    virtual binary doEncode(PVOID data, int size);
    virtual binary doDecode(PVOID data, int size);
protected:
    virtual const char* charTable();
};

///////////////////////////////////////////////////////////////////////////////
// Hash

class Hash
{
public:
    Hash();
    virtual ~Hash();

    void init();
    virtual void calc(PVOID data, int dataSize);
    void done();

    virtual PBYTE digest() = 0;
    virtual binary digestStr(FORMAT_TYPE formatType = FT_COPY);

    virtual int digestSize() = 0;
    virtual int blockSize() = 0;

    binary calcBuffer(PVOID buffer, int bufferSize, FORMAT_TYPE formatType = FT_COPY);
    binary calcStream(Stream& stream, INT64 size, FORMAT_TYPE formatType = FT_COPY, DataAlgoProgress *progress = NULL);
    binary calcBinary(const binary& data, FORMAT_TYPE formatType = FT_COPY);
    binary calcFile(char *fileName, FORMAT_TYPE formatType = FT_COPY, DataAlgoProgress *progress = NULL);

    BYTE& paddingByte() { return paddingByte_; }

protected:
    virtual void doTransform(DWORD *buffer) = 0;
    virtual void doInit() = 0;
    virtual void doDone() = 0;

protected:
    DWORD count_[8];
    BYTE *buffer_;
    int bufferSize_;
    int bufferIndex_;
    BYTE paddingByte_;
};

///////////////////////////////////////////////////////////////////////////////
// HashBaseMD4

class HashBaseMD4 : public Hash
{
public:
    HashBaseMD4();

    virtual int digestSize() { return 16; }
    virtual int blockSize() { return 64; }
    virtual PBYTE digest() { return (PBYTE)digest_; }

protected:
    virtual void doInit();
    virtual void doDone();

protected:
    DWORD digest_[10];
};

///////////////////////////////////////////////////////////////////////////////
// Hash_MD4

class Hash_MD4 : public HashBaseMD4
{
protected:
    virtual void doTransform(DWORD *buffer);
};

///////////////////////////////////////////////////////////////////////////////
// Hash_MD5

class Hash_MD5 : public HashBaseMD4
{
protected:
    virtual void doTransform(DWORD *buffer);
};

///////////////////////////////////////////////////////////////////////////////
// Hash_SHA

class Hash_SHA : public HashBaseMD4
{
public:
    Hash_SHA();
    virtual int digestSize() { return 20; }
protected:
    virtual void doTransform(DWORD *buffer);
    virtual void doDone();
protected:
    bool rotate_;
};

///////////////////////////////////////////////////////////////////////////////
// Hash_SHA1

class Hash_SHA1 : public Hash_SHA
{
protected:
    virtual void doTransform(DWORD *buffer);
};

///////////////////////////////////////////////////////////////////////////////
// Cipher

class Cipher
{
public:
    Cipher();
    virtual ~Cipher();

    virtual CipherContext context() = 0;

    void init(PVOID key, int size, PVOID iVector, int iVectorSize, BYTE iFiller = 0xFF);
    void init(const binary& key, const binary& iVector = "", BYTE iFiller = 0xFF);
    void done();
    virtual void protect();

    void encode(PVOID source, PVOID dest, int dataSize);
    void decode(PVOID source, PVOID dest, int dataSize);

    binary encodeBinary(const binary& sourceStr, FORMAT_TYPE formatType = FT_COPY);
    binary decodeBinary(const binary& sourceStr, FORMAT_TYPE formatType = FT_COPY);
    void encodeFile(char *srcFileName, char *destFileName, DataAlgoProgress *progress = NULL);
    void decodeFile(char *srcFileName, char *destFileName, DataAlgoProgress *progress = NULL);
    void encodeStream(Stream& srcStream, Stream& destStream, INT64 dataSize, DataAlgoProgress *progress = NULL);
    void decodeStream(Stream& srcStream, Stream& destStream, INT64 dataSize, DataAlgoProgress *progress = NULL);

    int getInitVectorSize() { return bufferSize_; }
    PBYTE getInitVector() { return vector_; }
    PBYTE getFeedback() { return feedback_; }

    CIPHER_STATE getState() { return state_; }
    CIPHER_MODE getMode() { return mode_; }
    void setMode(CIPHER_MODE value);

protected:
    virtual void doInit(PVOID key, int size) = 0;
    virtual void doEncode(PVOID source, PVOID dest, int size) = 0;
    virtual void doDecode(PVOID source, PVOID dest, int size) = 0;

protected:
    void checkState(DWORD states);
    void ensureInternalInit();

    void encodeECBx(PBYTE s, PBYTE d, int size);
    void encodeCBCx(PBYTE s, PBYTE d, int size);
    void encodeCTSx(PBYTE s, PBYTE d, int size);
    void encodeCFB8(PBYTE s, PBYTE d, int size);
    void encodeCFBx(PBYTE s, PBYTE d, int size);
    void encodeOFB8(PBYTE s, PBYTE d, int size);
    void encodeOFBx(PBYTE s, PBYTE d, int size);
    void encodeCFS8(PBYTE s, PBYTE d, int size);
    void encodeCFSx(PBYTE s, PBYTE d, int size);

    void decodeECBx(PBYTE s, PBYTE d, int size);
    void decodeCBCx(PBYTE s, PBYTE d, int size);
    void decodeCTSx(PBYTE s, PBYTE d, int size);
    void decodeCFB8(PBYTE s, PBYTE d, int size);
    void decodeCFBx(PBYTE s, PBYTE d, int size);
    void decodeOFB8(PBYTE s, PBYTE d, int size);
    void decodeOFBx(PBYTE s, PBYTE d, int size);
    void decodeCFS8(PBYTE s, PBYTE d, int size);
    void decodeCFSx(PBYTE s, PBYTE d, int size);

    void doCodeStream(Stream& srcStream, Stream& destStream, INT64 size, int blockSize,
        CIPHER_KIND cipherKind, DataAlgoProgress *progress);
    void doCodeFile(char *srcFileName, char *destFileName, int blockSize,
        CIPHER_KIND cipherKind, DataAlgoProgress *progress);

private:
    CIPHER_STATE state_;
    CIPHER_MODE mode_;
    PBYTE data_;
    int dataSize_;

protected:
    int bufferSize_;
    int bufferIndex_;
    int userSize_;
    PBYTE buffer_;
    PBYTE vector_;
    PBYTE feedback_;
    PBYTE user_;
    PBYTE userSave_;
};

///////////////////////////////////////////////////////////////////////////////
// Cipher_Null

class Cipher_Null : public Cipher
{
public:
    virtual CipherContext context();
protected:
    virtual void doInit(PVOID key, int size);
    virtual void doEncode(PVOID source, PVOID dest, int size);
    virtual void doDecode(PVOID source, PVOID dest, int size);
};

///////////////////////////////////////////////////////////////////////////////
// Cipher_Blowfish

class Cipher_Blowfish : public Cipher
{
public:
    virtual CipherContext context();
protected:
    virtual void doInit(PVOID key, int size);
    virtual void doEncode(PVOID source, PVOID dest, int size);
    virtual void doDecode(PVOID source, PVOID dest, int size);
};

///////////////////////////////////////////////////////////////////////////////
// Cipher_IDEA

class Cipher_IDEA : public Cipher
{
public:
    virtual CipherContext context();
protected:
    virtual void doInit(PVOID key, int size);
    virtual void doEncode(PVOID source, PVOID dest, int size);
    virtual void doDecode(PVOID source, PVOID dest, int size);
};

///////////////////////////////////////////////////////////////////////////////
// Cipher_DES

class Cipher_DES : public Cipher
{
public:
    virtual CipherContext context();
protected:
    virtual void doInit(PVOID key, int size);
    virtual void doEncode(PVOID source, PVOID dest, int size);
    virtual void doDecode(PVOID source, PVOID dest, int size);
protected:
    void DoInitKey(PBYTE data, PDWORD key, bool reverse);
};

///////////////////////////////////////////////////////////////////////////////
// Cipher_Gost

class Cipher_Gost : public Cipher
{
public:
    virtual CipherContext context();
protected:
    virtual void doInit(PVOID key, int size);
    virtual void doEncode(PVOID source, PVOID dest, int size);
    virtual void doDecode(PVOID source, PVOID dest, int size);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace utils

} // namespace ise

#endif // _ISE_EXT_UTILS_CIPHER_H_
