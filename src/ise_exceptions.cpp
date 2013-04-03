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
// 文件名称: ise_exceptions.cpp
// 功能描述: 异常类
///////////////////////////////////////////////////////////////////////////////

#include "ise_exceptions.h"
#include "ise_sysutils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////

const char* const EXCEPTION_LOG_PREFIX = "ERROR: ";

///////////////////////////////////////////////////////////////////////////////
// class CException

//-----------------------------------------------------------------------------
// 描述: 用于 std::exception 返回错误信息
//-----------------------------------------------------------------------------
const char* CException::what() const throw()
{
	m_strWhat = GetErrorMessage();
	return m_strWhat.c_str();
}

//-----------------------------------------------------------------------------
// 描述: 返回用于 Log 的字符串
//-----------------------------------------------------------------------------
string CException::MakeLogStr() const
{
	return string(EXCEPTION_LOG_PREFIX) + GetErrorMessage();
}

///////////////////////////////////////////////////////////////////////////////
// class CSimpleException

CSimpleException::CSimpleException(const char *lpszErrorMsg,
	const char *lpszSrcFileName, int nSrcLineNumber)
{
	if (lpszErrorMsg)
		m_strErrorMsg = lpszErrorMsg;
	if (lpszSrcFileName)
		m_strSrcFileName = lpszSrcFileName;
	m_nSrcLineNumber = nSrcLineNumber;
}

//-----------------------------------------------------------------------------

string CSimpleException::MakeLogStr() const
{
	string strResult(GetErrorMessage());

	if (!m_strSrcFileName.empty() && m_nSrcLineNumber >= 0)
		strResult = strResult + " (" + m_strSrcFileName + ":" + IntToStr(m_nSrcLineNumber) + ")";

	strResult = string(EXCEPTION_LOG_PREFIX) + strResult;

	return strResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CFileException

CFileException::CFileException(const char *lpszFileName, int nErrorCode, const char *lpszErrorMsg) :
	m_strFileName(lpszFileName),
	m_nErrorCode(nErrorCode)
{
	if (lpszErrorMsg == NULL)
		m_strErrorMsg = SysErrorMessage(nErrorCode);
	else
		m_strErrorMsg = lpszErrorMsg;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
