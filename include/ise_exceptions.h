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
// ise_exceptions.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EXCEPTIONS_H_
#define _ISE_EXCEPTIONS_H_

#include "ise_options.h"

#include "ise_global_defs.h"
#include "ise_errmsgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CException - “Ï≥£ª˘¿‡

class CException : std::exception
{
private:
	mutable string m_strWhat;
public:
	CException() {}
	virtual ~CException() throw() {}

	virtual const char* what() const throw();
	virtual string GetErrorMessage() const { return ""; }
	virtual string MakeLogStr() const;
};

///////////////////////////////////////////////////////////////////////////////
// CSimpleException - Simple exception

class CSimpleException : public CException
{
protected:
	string m_strErrorMsg;
	string m_strSrcFileName;
	int m_nSrcLineNumber;
public:
	CSimpleException() : m_nSrcLineNumber(-1) {}
	CSimpleException(const char *lpszErrorMsg,
		const char *lpszSrcFileName = NULL,
		int nSrcLineNumber = -1);
	virtual ~CSimpleException() throw() {}

	virtual string GetErrorMessage() const { return m_strErrorMsg; }
	virtual string MakeLogStr() const;
	void SetErrorMesssage(const char *lpszMsg) { m_strErrorMsg = lpszMsg; }
};

///////////////////////////////////////////////////////////////////////////////
// CMemoryException - Memory exception

class CMemoryException : public CSimpleException
{
public:
	CMemoryException() {}
	explicit CMemoryException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CMemoryException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CStreamException - Stream exception

class CStreamException : public CSimpleException
{
public:
	CStreamException() {}
	explicit CStreamException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CStreamException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CFileException - File exception

class CFileException : public CSimpleException
{
protected:
	string m_strFileName;
	int m_nErrorCode;
public:
	CFileException() : m_nErrorCode(0) {}
	CFileException(const char *lpszFileName, int nErrorCode, const char *lpszErrorMsg = NULL);
	virtual ~CFileException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CThreadException - Thread exception

class CThreadException : public CSimpleException
{
public:
	CThreadException() {}
	explicit CThreadException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CThreadException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CSocketException - Socket exception

class CSocketException : public CSimpleException
{
public:
	CSocketException() {}
	explicit CSocketException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CSocketException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CDbException - Database exception

class CDbException : public CSimpleException
{
public:
	CDbException() {}
	explicit CDbException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CDbException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// CDataAlgoException - DataAlgo exception

class CDataAlgoException : public CSimpleException
{
public:
	CDataAlgoException() {}
	explicit CDataAlgoException(const char *lpszErrorMsg) : CSimpleException(lpszErrorMsg) {}
	virtual ~CDataAlgoException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// Exception throwing rountines.

#define ISE_THROW_EXCEPTION(msg)  IseThrowException(msg, __FILE__, __LINE__)

// Throws a CSimpleException exception.
inline void IseThrowException(const char *lpszMsg,
	const char *lpszSrcFileName = NULL, int nSrcLineNumber = -1)
{
	throw CSimpleException(lpszMsg, lpszSrcFileName, nSrcLineNumber);
}

// Throws a CMemoryException exception.
inline void IseThrowMemoryException()
{
	throw CMemoryException(SEM_OUT_OF_MEMORY);
}

// Throws a CStreamException exception.
inline void IseThrowStreamException(const char *lpszMsg)
{
	throw CStreamException(lpszMsg);
}

// Throws a CFileException exception.
inline void IseThrowFileException(const char *lpszFileName, int nErrorCode,
	const char *lpszErrorMsg = NULL)
{
	throw CFileException(lpszFileName, nErrorCode, lpszErrorMsg);
}

// Throws a CThreadException exception.
inline void IseThrowThreadException(const char *lpszMsg)
{
	throw CThreadException(lpszMsg);
}

// Throws a CSocketException exception.
inline void IseThrowSocketException(const char *lpszMsg)
{
	throw CSocketException(lpszMsg);
}

// Throws a CDbException exception.
inline void IseThrowDbException(const char *lpszMsg)
{
	throw CDbException(lpszMsg);
}

// Throws a CDataAlgoException exception.
inline void IseThrowDataAlgoException(const char *lpszMsg)
{
	throw new CDataAlgoException(lpszMsg);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_EXCEPTIONS_H_
