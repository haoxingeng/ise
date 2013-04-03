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
// ise_data_algo.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_DATA_ALGO_H_
#define _ISE_DATA_ALGO_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_classes.h"
#include "ise_exceptions.h"

#include <string>
#include <memory>

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class declares

class CFormat;
class CFormat_Copy;
class CFormat_HEX;
class CFormat_HEXL;
class CFormat_MIME32;
class CFormat_MIME64;

class CHash;
class CHashBaseMD4;
class CHash_MD4;
class CHash_MD5;
class CHash_SHA;
class CHash_SHA1;

class CCipher;
class CCipher_Null;
class CCipher_Blowfish;
class CCipher_IDEA;
class CCipher_DES;
class CCipher_Gost;

///////////////////////////////////////////////////////////////////////////////
// Type Definitions

typedef std::string binary;

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
struct CCipherContext
{
	int nKeySize;               // maximal key size in bytes
	int nBlockSize;             // mininmal block size in bytes, eg. 1 = Streamcipher
	int nBufferSize;            // internal buffersize in bytes
	int nUserSize;              // internal size in bytes of cipher dependend structures
	bool bUserSave;
};

///////////////////////////////////////////////////////////////////////////////
// Const Definitions

const CIPHER_MODE DEFAULT_CIPHER_MODE = CM_CTSx;

///////////////////////////////////////////////////////////////////////////////
// Misc Routines

binary HashString(HASH_TYPE nHashType, const binary& strSource, PVOID pDigest = NULL);
binary HashBuffer(HASH_TYPE nHashType, PVOID pBuffer, int nDataSize, PVOID pDigest = NULL);
binary HashStream(HASH_TYPE nHashType, CStream& Stream, PVOID pDigest = NULL);
binary HashFile(HASH_TYPE nHashType, char *lpszFileName, PVOID pDigest = NULL);

DWORD CalcCrc32(PVOID pData, int nDataSize, DWORD nLastResult = 0xFFFFFFFF, CRC32_TYPE nCrc32Type = CRC32_SPECIAL);
BYTE CalcCrc8(PVOID pData, int nDataSize);

void EncryptBuffer(CIPHER_TYPE nCipherType, PVOID pSource, PVOID pDest, int nDataSize, char *lpszKey);
void DecryptBuffer(CIPHER_TYPE nCipherType, PVOID pSource, PVOID pDest, int nDataSize, char *lpszKey);
void EncryptStream(CIPHER_TYPE nCipherType, CStream& SrcStream, CStream& DestStream, char *lpszKey);
void DecryptStream(CIPHER_TYPE nCipherType, CStream& SrcStream, CStream& DestStream, char *lpszKey);
void EncryptFile(CIPHER_TYPE nCipherType, char *lpszSrcFileName, char *lpszDestFileName, char *lpszKey);
void DecryptFile(CIPHER_TYPE nCipherType, char *lpszSrcFileName, char *lpszDestFileName, char *lpszKey);

binary Base64Encode(PVOID pData, int nDataSize);
binary Base64Decode(PVOID pData, int nDataSize);
binary Base16Encode(PVOID pData, int nDataSize);
binary Base16Decode(PVOID pData, int nDataSize);

///////////////////////////////////////////////////////////////////////////////
// IDataAlgoProgress

class IDataAlgoProgress
{
public:
	virtual void Progress(INT64 nMin, INT64 nMax, INT64 nPos) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// CFormat

class CFormat
{
protected:
	virtual binary DoEncode(PVOID pData, int nSize) = 0;
	virtual binary DoDecode(PVOID pData, int nSize) = 0;
	virtual bool DoIsValid(PVOID pData, int nSize) = 0;
public:
	virtual ~CFormat() {}
	binary Encode(const binary& strValue);
	binary Encode(PVOID pData, int nSize);
	binary Decode(const binary& strValue);
	binary Decode(PVOID pData, int nSize);
	bool IsValid(const binary& strValue);
	bool IsValid(PVOID pData, int nSize);
};

///////////////////////////////////////////////////////////////////////////////
// CFormat_Copy

class CFormat_Copy : public CFormat
{
protected:
	virtual binary DoEncode(PVOID pData, int nSize);
	virtual binary DoDecode(PVOID pData, int nSize);
	virtual bool DoIsValid(PVOID pData, int nSize);
};

///////////////////////////////////////////////////////////////////////////////
// CFormat_HEX

class CFormat_HEX : public CFormat
{
protected:
	virtual binary DoEncode(PVOID pData, int nSize);
	virtual binary DoDecode(PVOID pData, int nSize);
	virtual bool DoIsValid(PVOID pData, int nSize);
protected:
	virtual const char* CharTable();
};

///////////////////////////////////////////////////////////////////////////////
// CFormat_HEXL

class CFormat_HEXL : public CFormat_HEX
{
protected:
	virtual const char* CharTable();
};

///////////////////////////////////////////////////////////////////////////////
// CFormat_MIME32

class CFormat_MIME32 : public CFormat_HEX
{
protected:
	virtual binary DoEncode(PVOID pData, int nSize);
	virtual binary DoDecode(PVOID pData, int nSize);
protected:
	virtual const char* CharTable();
};

///////////////////////////////////////////////////////////////////////////////
// CFormat_MIME64

class CFormat_MIME64 : public CFormat_HEX
{
protected:
	virtual binary DoEncode(PVOID pData, int nSize);
	virtual binary DoDecode(PVOID pData, int nSize);
protected:
	virtual const char* CharTable();
};

///////////////////////////////////////////////////////////////////////////////
// CHash

class CHash
{
protected:
	DWORD m_nCount[8];
	BYTE *m_pBuffer;
	int m_nBufferSize;
	int m_nBufferIndex;
	BYTE m_nPaddingByte;
protected:
	virtual void DoTransform(DWORD *pBuffer) = 0;
	virtual void DoInit() = 0;
	virtual void DoDone() = 0;
public:
	CHash();
	virtual ~CHash();

	void Init();
	virtual void Calc(PVOID pData, int nDataSize);
	void Done();

	virtual PBYTE Digest() = 0;
	virtual binary DigestStr(FORMAT_TYPE nFormatType = FT_COPY);

	virtual int DigestSize() = 0;
	virtual int BlockSize() = 0;

	binary CalcBuffer(PVOID pBuffer, int nBufferSize, FORMAT_TYPE nFormatType = FT_COPY);
	binary CalcStream(CStream& Stream, INT64 nSize, FORMAT_TYPE nFormatType = FT_COPY, IDataAlgoProgress *pProgress = NULL);
	binary CalcBinary(const binary& strData, FORMAT_TYPE nFormatType = FT_COPY);
	binary CalcFile(char *lpszFileName, FORMAT_TYPE nFormatType = FT_COPY, IDataAlgoProgress *pProgress = NULL);

	BYTE& PaddingByte() { return m_nPaddingByte; }
};

///////////////////////////////////////////////////////////////////////////////
// CHashBaseMD4

class CHashBaseMD4 : public CHash
{
protected:
	DWORD m_nDigest[10];
protected:
	virtual void DoInit();
	virtual void DoDone();
public:
	CHashBaseMD4();

	virtual int DigestSize() { return 16; }
	virtual int BlockSize() { return 64; }
	virtual PBYTE Digest() { return (PBYTE)m_nDigest; }
};

///////////////////////////////////////////////////////////////////////////////
// CHash_MD4

class CHash_MD4 : public CHashBaseMD4
{
protected:
	virtual void DoTransform(DWORD *pBuffer);
};

///////////////////////////////////////////////////////////////////////////////
// CHash_MD5

class CHash_MD5 : public CHashBaseMD4
{
protected:
	virtual void DoTransform(DWORD *pBuffer);
};

///////////////////////////////////////////////////////////////////////////////
// CHash_SHA

class CHash_SHA : public CHashBaseMD4
{
protected:
	bool m_bRotate;
protected:
	virtual void DoTransform(DWORD *pBuffer);
	virtual void DoDone();
public:
	CHash_SHA();
	virtual int DigestSize() { return 20; }
};

///////////////////////////////////////////////////////////////////////////////
// CHash_SHA1

class CHash_SHA1 : public CHash_SHA
{
protected:
	virtual void DoTransform(DWORD *pBuffer);
};

///////////////////////////////////////////////////////////////////////////////
// CCipher

class CCipher
{
private:
	CIPHER_STATE m_nState;
	CIPHER_MODE m_nMode;
	PBYTE m_pData;
	int m_nDataSize;
protected:
	int m_nBufferSize;
	int m_nBufferIndex;
	int m_nUserSize;
	PBYTE m_pBuffer;
	PBYTE m_pVector;
	PBYTE m_pFeedback;
	PBYTE m_pUser;
	PBYTE m_pUserSave;
protected:
	void CheckState(DWORD nStates);
	void EnsureInternalInit();

	void EncodeECBx(PBYTE s, PBYTE d, int nSize);
	void EncodeCBCx(PBYTE s, PBYTE d, int nSize);
	void EncodeCTSx(PBYTE s, PBYTE d, int nSize);
	void EncodeCFB8(PBYTE s, PBYTE d, int nSize);
	void EncodeCFBx(PBYTE s, PBYTE d, int nSize);
	void EncodeOFB8(PBYTE s, PBYTE d, int nSize);
	void EncodeOFBx(PBYTE s, PBYTE d, int nSize);
	void EncodeCFS8(PBYTE s, PBYTE d, int nSize);
	void EncodeCFSx(PBYTE s, PBYTE d, int nSize);

	void DecodeECBx(PBYTE s, PBYTE d, int nSize);
	void DecodeCBCx(PBYTE s, PBYTE d, int nSize);
	void DecodeCTSx(PBYTE s, PBYTE d, int nSize);
	void DecodeCFB8(PBYTE s, PBYTE d, int nSize);
	void DecodeCFBx(PBYTE s, PBYTE d, int nSize);
	void DecodeOFB8(PBYTE s, PBYTE d, int nSize);
	void DecodeOFBx(PBYTE s, PBYTE d, int nSize);
	void DecodeCFS8(PBYTE s, PBYTE d, int nSize);
	void DecodeCFSx(PBYTE s, PBYTE d, int nSize);

	void DoCodeStream(CStream& SrcStream, CStream& DestStream, INT64 nSize, int nBlockSize,
		CIPHER_KIND nCipherKind, IDataAlgoProgress *pProgress);
	void DoCodeFile(char *lpszSrcFileName, char *lpszDestFileName, int nBlockSize,
		CIPHER_KIND nCipherKind, IDataAlgoProgress *pProgress);

protected:
	virtual void DoInit(PVOID pKey, int nSize) = 0;
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize) = 0;
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize) = 0;

public:
	CCipher();
	virtual ~CCipher();

	virtual CCipherContext Context() = 0;

	void Init(PVOID pKey, int nSize, PVOID pIVector, int nIVectorSize, BYTE nIFiller = 0xFF);
	void Init(const binary& strKey, const binary& strIVector = "", BYTE nIFiller = 0xFF);
	void Done();
	virtual void Protect();

	void Encode(PVOID pSource, PVOID pDest, int nDataSize);
	void Decode(PVOID pSource, PVOID pDest, int nDataSize);

	binary EncodeBinary(const binary& strSource, FORMAT_TYPE nFormatType = FT_COPY);
	binary DecodeBinary(const binary& strSource, FORMAT_TYPE nFormatType = FT_COPY);
	void EncodeFile(char *lpszSrcFileName, char *lpszDestFileName, IDataAlgoProgress *pProgress = NULL);
	void DecodeFile(char *lpszSrcFileName, char *lpszDestFileName, IDataAlgoProgress *pProgress = NULL);
	void EncodeStream(CStream& SrcStream, CStream& DestStream, INT64 nDataSize, IDataAlgoProgress *pProgress = NULL);
	void DecodeStream(CStream& SrcStream, CStream& DestStream, INT64 nDataSize, IDataAlgoProgress *pProgress = NULL);

	int GetInitVectorSize() { return m_nBufferSize; }
	PBYTE GetInitVector() { return m_pVector; }
	PBYTE GetFeedback() { return m_pFeedback; }

	CIPHER_STATE GetState() { return m_nState; }
	CIPHER_MODE GetMode() { return m_nMode; }
	void SetMode(CIPHER_MODE nValue);
};

///////////////////////////////////////////////////////////////////////////////
// CCipher_Null

class CCipher_Null : public CCipher
{
protected:
	virtual void DoInit(PVOID pKey, int nSize);
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize);
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize);
public:
	virtual CCipherContext Context();
};

///////////////////////////////////////////////////////////////////////////////
// CCipher_Blowfish

class CCipher_Blowfish : public CCipher
{
protected:
	virtual void DoInit(PVOID pKey, int nSize);
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize);
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize);
public:
	virtual CCipherContext Context();
};

///////////////////////////////////////////////////////////////////////////////
// CCipher_IDEA

class CCipher_IDEA : public CCipher
{
protected:
	virtual void DoInit(PVOID pKey, int nSize);
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize);
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize);
public:
	virtual CCipherContext Context();
};

///////////////////////////////////////////////////////////////////////////////
// CCipher_DES

class CCipher_DES : public CCipher
{
protected:
	void DoInitKey(PBYTE pData, PDWORD pKey, bool bReverse);
protected:
	virtual void DoInit(PVOID pKey, int nSize);
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize);
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize);
public:
	virtual CCipherContext Context();
};

///////////////////////////////////////////////////////////////////////////////
// CCipher_Gost

class CCipher_Gost : public CCipher
{
protected:
	virtual void DoInit(PVOID pKey, int nSize);
	virtual void DoEncode(PVOID pSource, PVOID pDest, int nSize);
	virtual void DoDecode(PVOID pSource, PVOID pDest, int nSize);
public:
	virtual CCipherContext Context();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DATA_ALGO_H_
