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
// 文件名称: ise_sysutils.cpp
// 功能描述: 系统杂项功能
///////////////////////////////////////////////////////////////////////////////

#include "ise_sysutils.h"
#include "ise_classes.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个整数
//-----------------------------------------------------------------------------
bool IsIntStr(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
		strtol(pStr, &endp, 10);
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个整数
//-----------------------------------------------------------------------------
bool IsInt64Str(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
#ifdef ISE_WIN32
		_strtoi64(pStr, &endp, 10);
#endif
#ifdef ISE_LINUX
		strtoll(pStr, &endp, 10);
#endif
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个浮点数
//-----------------------------------------------------------------------------
bool IsFloatStr(const string& str)
{
	bool bResult;
	int nLen = (int)str.size();
	char *pStr = (char*)str.c_str();

	bResult = (nLen > 0) && !isspace(pStr[0]);

	if (bResult)
	{
		char *endp;
		strtod(pStr, &endp);
		bResult = (endp - pStr == nLen);
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串可否转换成布尔型
//-----------------------------------------------------------------------------
bool IsBoolStr(const string& str)
{
	return SameText(str, TRUE_STR) || SameText(str, FALSE_STR) || IsFloatStr(str);
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成整型(若转换失败，则返回 nDefault)
//-----------------------------------------------------------------------------
int StrToInt(const string& str, int nDefault)
{
	if (IsIntStr(str))
		return strtol(str.c_str(), NULL, 10);
	else
		return nDefault;
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成64位整型(若转换失败，则返回 nDefault)
//-----------------------------------------------------------------------------
INT64 StrToInt64(const string& str, INT64 nDefault)
{
	if (IsInt64Str(str))
#ifdef ISE_WIN32
		return _strtoi64(str.c_str(), NULL, 10);
#endif
#ifdef ISE_LINUX
		return strtol(str.c_str(), NULL, 10);
#endif
	else
		return nDefault;
}

//-----------------------------------------------------------------------------
// 描述: 整型转换成字符串
//-----------------------------------------------------------------------------
string IntToStr(int nValue)
{
	char sTemp[64];
	sprintf(sTemp, "%d", nValue);
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// 描述: 64位整型转换成字符串
//-----------------------------------------------------------------------------
string IntToStr(INT64 nValue)
{
	char sTemp[64];
#ifdef ISE_WIN32
	sprintf(sTemp, "%I64d", nValue);
#endif
#ifdef ISE_LINUX
	sprintf(sTemp, "%lld", nValue);
#endif
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成浮点型(若转换失败，则返回 fDefault)
//-----------------------------------------------------------------------------
double StrToFloat(const string& str, double fDefault)
{
	if (IsFloatStr(str))
		return strtod(str.c_str(), NULL);
	else
		return fDefault;
}

//-----------------------------------------------------------------------------
// 描述: 浮点型转换成字符串
//-----------------------------------------------------------------------------
string FloatToStr(double fValue, const char *sFormat)
{
	char sTemp[256];
	sprintf(sTemp, sFormat, fValue);
	return &sTemp[0];
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成布尔型
//-----------------------------------------------------------------------------
bool StrToBool(const string& str, bool bDefault)
{
	if (IsBoolStr(str))
	{
		if (SameText(str, TRUE_STR))
			return true;
		else if (SameText(str, FALSE_STR))
			return false;
		else
			return (StrToFloat(str, 0) != 0);
	}
	else
		return bDefault;
}

//-----------------------------------------------------------------------------
// 描述: 布尔型转换成字符串
//-----------------------------------------------------------------------------
string BoolToStr(bool bValue, bool bUseBoolStrs)
{
	if (bUseBoolStrs)
		return (bValue? TRUE_STR : FALSE_STR);
	else
		return (bValue? "1" : "0");
}

//-----------------------------------------------------------------------------
// 描述: 格式化字符串 (供FormatString函数调用)
//-----------------------------------------------------------------------------
void FormatStringV(string& strResult, const char *sFormatString, va_list argList)
{
#if defined(ISE_COMPILER_VC)

	int nSize = _vscprintf(sFormatString, argList);
	char *pBuffer = (char *)malloc(nSize + 1);
	if (pBuffer)
	{
		vsprintf(pBuffer, sFormatString, argList);
		strResult = pBuffer;
		free(pBuffer);
	}
	else
		strResult = "";

#else

	int nSize = 100;
	char *pBuffer = (char *)malloc(nSize);
	va_list args;

	while (pBuffer)
	{
		int nChars;

		va_copy(args, argList);
		nChars = vsnprintf(pBuffer, nSize, sFormatString, args);
		va_end(args);

		if (nChars > -1 && nChars < nSize)
			break;
		if (nChars > -1)
			nSize = nChars + 1;
		else
			nSize *= 2;
		pBuffer = (char *)realloc(pBuffer, nSize);
	}

	if (pBuffer)
	{
		strResult = pBuffer;
		free(pBuffer);
	}
	else
		strResult = "";

#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回格式化后的字符串
// 参数:
//   sFormatString  - 格式化字符串
//   ...            - 格式化参数
//-----------------------------------------------------------------------------
string FormatString(const char *sFormatString, ...)
{
	string strResult;

	va_list argList;
	va_start(argList, sFormatString);
	FormatStringV(strResult, sFormatString, argList);
	va_end(argList);

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 判断两个字符串是否相同 (不区分大小写)
//-----------------------------------------------------------------------------
bool SameText(const string& str1, const string& str2)
{
	return CompareText(str1.c_str(), str2.c_str()) == 0;
}

//-----------------------------------------------------------------------------
// 描述: 比较两个字符串 (不区分大小写)
//-----------------------------------------------------------------------------
int CompareText(const char* str1, const char* str2)
{
#ifdef ISE_COMPILER_VC
	return _stricmp(str1, str2);
#endif
#ifdef ISE_COMPILER_BCB
	return stricmp(str1, str2);
#endif
#ifdef ISE_COMPILER_GCC
	return strcasecmp(str1, str2);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 去掉字符串头尾的空白字符 (ASCII <= 32)
//-----------------------------------------------------------------------------
string TrimString(const string& str)
{
	string strResult;
	int i, nLen;

	nLen = (int)str.size();
	i = 0;
	while (i < nLen && (BYTE)str[i] <= 32) i++;
	if (i < nLen)
	{
		while ((BYTE)str[nLen-1] <= 32) nLen--;
		strResult = str.substr(i, nLen - i);
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变大写
//-----------------------------------------------------------------------------
string UpperCase(const string& str)
{
	string strResult = str;
	int nLen = (int)strResult.size();
	char c;

	for (int i = 0; i < nLen; i++)
	{
		c = strResult[i];
		if (c >= 'a' && c <= 'z')
			strResult[i] = c - 32;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变小写
//-----------------------------------------------------------------------------
string LowerCase(const string& str)
{
	string strResult = str;
	int nLen = (int)strResult.size();
	char c;

	for (int i = 0; i < nLen; i++)
	{
		c = strResult[i];
		if (c >= 'A' && c <= 'Z')
			strResult[i] = c + 32;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 字符串替换
// 参数:
//   strSource        - 源串
//   strOldPattern    - 源串中将被替换的字符串
//   strNewPattern    - 取代 strOldPattern 的字符串
//   bReplaceAll      - 是否替换源串中所有匹配的字符串(若为false，则只替换第一处)
//   bCaseSensitive   - 是否区分大小写
// 返回:
//   进行替换动作之后的字符串
//-----------------------------------------------------------------------------
string RepalceString(const string& strSource, const string& strOldPattern,
	const string& strNewPattern, bool bReplaceAll, bool bCaseSensitive)
{
	string strResult = strSource;
	string strSearch, strPattern;
	string::size_type nOffset, nIndex;
	int nOldPattLen, nNewPattLen;

	if (!bCaseSensitive)
	{
		strSearch = UpperCase(strSource);
		strPattern = UpperCase(strOldPattern);
	}
	else
	{
		strSearch = strSource;
		strPattern = strOldPattern;
	}

	nOldPattLen = (int)strOldPattern.size();
	nNewPattLen = (int)strNewPattern.size();
	nIndex = 0;

	while (nIndex < strSearch.size())
	{
		nOffset = strSearch.find(strPattern, nIndex);
		if (nOffset == string::npos) break;  // 若没找到

		strSearch.replace(nOffset, nOldPattLen, strNewPattern);
		strResult.replace(nOffset, nOldPattLen, strNewPattern);
		nIndex = (nOffset + nNewPattLen);

		if (!bReplaceAll) break;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 分割字符串
// 参数:
//   strSource   - 源串
//   chSplitter  - 分隔符
//   StrList     - 存放分割之后的字符串列表
//   bTrimResult - 是否对分割后的结果进行 trim 处理
// 示例:
//   ""          -> []
//   " "         -> [" "]
//   ","         -> ["", ""]
//   "a,b,c"     -> ["a", "b", "c"]
//   ",a,,b,c,"  -> ["", "a", "", "b", "c", ""]
//-----------------------------------------------------------------------------
void SplitString(const string& strSource, char chSplitter, CStrList& StrList,
	bool bTrimResult)
{
	string::size_type nOffset, nIndex = 0;

	StrList.Clear();
	if (strSource.empty()) return;

	while (true)
	{
		nOffset = strSource.find(chSplitter, nIndex);
		if (nOffset == string::npos)   // 若没找到
		{
			StrList.Add(strSource.substr(nIndex).c_str());
			break;
		}
		else
		{
			StrList.Add(strSource.substr(nIndex, nOffset - nIndex).c_str());
			nIndex = nOffset + 1;
		}
	}

	if (bTrimResult)
	{
		for (int i = 0; i < StrList.GetCount(); i++)
			StrList.SetString(i, TrimString(StrList[i]).c_str());
	}
}

//-----------------------------------------------------------------------------
// 描述: 分割字符串并转换成整型数列表
// 参数:
//   strSource  - 源串
//   chSplitter - 分隔符
//   IntList    - 存放分割之后的整型数列表
//-----------------------------------------------------------------------------
void SplitStringToInt(const string& strSource, char chSplitter, IntegerArray& IntList)
{
	CStrList StrList;
	SplitString(strSource, chSplitter, StrList, true);

	IntList.clear();
	for (int i = 0; i < StrList.GetCount(); i++)
		IntList.push_back(atoi(StrList[i].c_str()));
}

//-----------------------------------------------------------------------------
// 描述: 复制串 pSource 到 pDest 中
// 备注:
//   1. 最多只复制 nMaxBytes 个字节到 pDest 中，包括结束符'\0'。
//   2. 如果 pSource 的实际长度(strlen)小于 nMaxBytes，则复制会提前结束，
//      pDest 的剩余部分以 '\0' 填充。
//   3. 如果 pSource 的实际长度(strlen)大于 nMaxBytes，则复制之后的 pDest 没有结束符。
//-----------------------------------------------------------------------------
char *StrNCopy(char *pDest, const char *pSource, int nMaxBytes)
{
	if (nMaxBytes > 0)
	{
		if (pSource)
			return strncpy(pDest, pSource, nMaxBytes);
		else
			return strcpy(pDest, "");
	}

	return pDest;
}

//-----------------------------------------------------------------------------
// 描述: 复制串 pSource 到 pDest 中
// 备注: 最多只复制 nDestSize 个字节到 pDest 中。并将 pDest 的最后字节设为'\0'。
// 参数:
//   nDestSize - pDest的大小
//-----------------------------------------------------------------------------
char *StrNZCopy(char *pDest, const char *pSource, int nDestSize)
{
	if (nDestSize > 0)
	{
		if (pSource)
		{
			char *p;
			p = strncpy(pDest, pSource, nDestSize);
			pDest[nDestSize - 1] = '\0';
			return p;
		}
		else
			return strcpy(pDest, "");
	}
	else
		return pDest;
}

//-----------------------------------------------------------------------------
// 描述: 从源串中获取一个子串
//
// For example:
//   strInput(before)   chDelimiter  bDelete       strInput(after)   Result(after)
//   ----------------   -----------  ----------    ---------------   -------------
//   "abc def"           ' '         false         "abc def"         "abc"
//   "abc def"           ' '         true          "def"             "abc"
//   " abc"              ' '         false         " abc"            ""
//   " abc"              ' '         true          "abc"             ""
//   ""                  ' '         true/false    ""                ""
//-----------------------------------------------------------------------------
string FetchStr(string& strInput, char chDelimiter, bool bDelete)
{
	string strResult;

	string::size_type nPos = strInput.find(chDelimiter, 0);
	if (nPos == string::npos)
	{
		strResult = strInput;
		if (bDelete)
			strInput.clear();
	}
	else
	{
		strResult = strInput.substr(0, nPos);
		if (bDelete)
			strInput = strInput.substr(nPos + 1);
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 在数字中间插入逗号进行数据分组
//-----------------------------------------------------------------------------
string AddThousandSep(const INT64& nNumber)
{
	string strResult = IntToStr(nNumber);
	for (int i = (int)strResult.length() - 3; i > 0; i -= 3)
		strResult.insert(i, ",");
	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a string to a quoted string.
// For example:
//    abc         ->     "abc"
//    ab'c        ->     "ab'c"
//    ab"c        ->     "ab""c"
//    (empty)     ->     ""
//-----------------------------------------------------------------------------
string GetQuotedStr(const char* lpszStr, char chQuote)
{
	string strResult;
	string strSrc(lpszStr);

	strResult = chQuote;

	string::size_type nStart = 0;
	while (true)
	{
		string::size_type nPos = strSrc.find(chQuote, nStart);
		if (nPos != string::npos)
		{
			strResult += strSrc.substr(nStart, nPos - nStart) + chQuote + chQuote;
			nStart = nPos + 1;
		}
		else
		{
			strResult += strSrc.substr(nStart);
			break;
		}
	}

	strResult += chQuote;

	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// ExtractQuotedStr removes the quote characters from the beginning and end of a quoted string,
// and reduces pairs of quote characters within the string to a single quote character.
// The @a chQuote parameter defines what character to use as a quote character. If the first
// character in @a lpszStr is not the value of the @a chQuote parameter, ExtractQuotedStr returns
// an empty string.
//
// The function copies characters from @a lpszStr to the result string until the second solitary
// quote character or the first null character in @a lpszStr. The @a lpszStr parameter is updated
// to point to the first character following the quoted string. If @a lpszStr does not contain a
// matching end quote character, the @a lpszStr parameter is updated to point to the terminating
// null character.
//
// For example:
//    lpszStr(before)    Returned string        lpszStr(after)
//    ---------------    ---------------        ---------------
//    "abc"               abc                    '\0'
//    "ab""c"             ab"c                   '\0'
//    "abc"123            abc                    123
//    abc"                (empty)                abc"
//    "abc                abc                    '\0'
//    (empty)             (empty)                '\0'
//-----------------------------------------------------------------------------
string ExtractQuotedStr(const char*& lpszStr, char chQuote)
{
	string strResult;
	const char* lpszStart = lpszStr;

	if (lpszStr == NULL || *lpszStr != chQuote)
		return "";

	// Calc the character count after converting.

	int nSize = 0;
	lpszStr++;
	while (*lpszStr != '\0')
	{
		if (lpszStr[0] == chQuote)
		{
			if (lpszStr[1] == chQuote)
			{
				nSize++;
				lpszStr += 2;
			}
			else
			{
				lpszStr++;
				break;
			}
		}
		else
		{
			const char* p = lpszStr;
			lpszStr++;
			nSize += (int)(lpszStr - p);
		}
	}

	// Begin to retrieve the characters.

	strResult.resize(nSize);
	char* pResult = (char*)strResult.c_str();
	lpszStr = lpszStart;
	lpszStr++;
	while (*lpszStr != '\0')
	{
		if (lpszStr[0] == chQuote)
		{
			if (lpszStr[1] == chQuote)
			{
				*pResult++ = *lpszStr;
				lpszStr += 2;
			}
			else
			{
				lpszStr++;
				break;
			}
		}
		else
		{
			const char* p = lpszStr;
			lpszStr++;
			while (p < lpszStr)
				*pResult++ = *p++;
		}
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// GetDequotedStr removes the quote characters from the beginning and end of a quoted string, and
// reduces pairs of quote characters within the string to a single quote character. The chQuote
// parameter defines what character to use as a quote character. If the @a lpszStr parameter does
// not begin and end with the quote character, GetDequotedStr returns @a lpszStr unchanged.
//
// For example:
//    "abc"     ->     abc
//    "ab""c"   ->     ab"c
//    "abc      ->     "abc
//    abc"      ->     abc"
//    (empty)   ->    (empty)
//-----------------------------------------------------------------------------
string GetDequotedStr(const char* lpszStr, char chQuote)
{
	const char* lpszStart = lpszStr;
	int nStrLen = (int)strlen(lpszStr);

	string strResult = ExtractQuotedStr(lpszStr, chQuote);

	if ( (strResult.empty() || *lpszStr == '\0') &&
		nStrLen > 0 && (lpszStart[0] != chQuote || lpszStart[nStrLen-1] != chQuote) )
		strResult = lpszStart;

	return strResult;
}

//-----------------------------------------------------------------------------

#ifdef ISE_WIN32
static bool GetFileFindData(const string& strFileName, WIN32_FIND_DATAA& FindData)
{
	HANDLE hFindHandle = ::FindFirstFileA(strFileName.c_str(), &FindData);
	bool bResult = (hFindHandle != INVALID_HANDLE_VALUE);
	if (bResult) ::FindClose(hFindHandle);
	return bResult;
}
#endif

//-----------------------------------------------------------------------------
// 描述: 检查文件是否存在
//-----------------------------------------------------------------------------
bool FileExists(const string& strFileName)
{
#ifdef ISE_WIN32
	DWORD nFileAttr = ::GetFileAttributesA(strFileName.c_str());
	if (nFileAttr != (DWORD)(-1))
		return ((nFileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
	else
	{
		WIN32_FIND_DATAA FindData;
		DWORD nLastError = ::GetLastError();
		return
			(nLastError != ERROR_FILE_NOT_FOUND) &&
			(nLastError != ERROR_PATH_NOT_FOUND) &&
			(nLastError != ERROR_INVALID_NAME) &&
			GetFileFindData(strFileName, FindData) &&
			(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
#endif
#ifdef ISE_LINUX
	return (euidaccess(strFileName.c_str(), F_OK) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 检查目录是否存在
//-----------------------------------------------------------------------------
bool DirectoryExists(const string& strDir)
{
#ifdef ISE_WIN32
	int nCode;
	nCode = GetFileAttributesA(strDir.c_str());
	return (nCode != -1) && ((FILE_ATTRIBUTE_DIRECTORY & nCode) != 0);
#endif
#ifdef ISE_LINUX
	string strPath = PathWithSlash(strDir);
	struct stat st;
	bool bResult;

	if (stat(strPath.c_str(), &st) == 0)
		bResult = ((st.st_mode & S_IFDIR) == S_IFDIR);
	else
		bResult = false;

	return bResult;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 创建目录
// 示例:
//   CreateDir("C:\\test");
//   CreateDir("/home/test");
//-----------------------------------------------------------------------------
bool CreateDir(const string& strDir)
{
#ifdef ISE_WIN32
	return CreateDirectoryA(strDir.c_str(), NULL) != 0;
#endif
#ifdef ISE_LINUX
	return mkdir(strDir.c_str(), (mode_t)(-1)) == 0;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 删除目录
// 参数:
//   strDir     - 待删除的目录
//   bRecursive - 是否递归删除
// 返回:
//   true   - 成功
//   false  - 失败
//-----------------------------------------------------------------------------
bool DeleteDir(const string& strDir, bool bRecursive)
{
	if (!bRecursive)
	{
#ifdef ISE_WIN32
		return RemoveDirectoryA(strDir.c_str()) != 0;
#endif
#ifdef ISE_LINUX
		return rmdir(strDir.c_str()) == 0;
#endif
	}

#ifdef ISE_WIN32
	const char* const ALL_FILE_WILDCHAR = "*.*";
#endif
#ifdef ISE_LINUX
	const char* const ALL_FILE_WILDCHAR = "*";
#endif

	bool bResult = true;
	string strPath = PathWithSlash(strDir);
	FILE_FIND_RESULT fr;
	FindFiles(strPath + ALL_FILE_WILDCHAR, FA_ANY_FILE, fr);

	for (int i = 0; i < (int)fr.size() && bResult; i++)
	{
		string strFullName = strPath + fr[i].strFileName;
		if (fr[i].nAttr & FA_DIRECTORY)
			bResult = DeleteDir(strFullName, true);
		else
			RemoveFile(strFullName);
	}

	bResult = DeleteDir(strPath, false);

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 取得文件名中最后一个分隔符的位置(0-based)。若没有，则返回-1
//-----------------------------------------------------------------------------
int GetLastDelimPos(const string& strFileName, const string& strDelims)
{
	int nResult = (int)strFileName.size() - 1;

	for (; nResult >= 0; nResult--)
		if (strDelims.find(strFileName[nResult], 0) != string::npos)
			break;

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件路径
// 参数:
//   strFileName - 包含路径的文件名
// 返回:
//   文件的路径
// 示例:
//   ExtractFilePath("C:\\MyDir\\test.c");         返回: "C:\\MyDir\\"
//   ExtractFilePath("C:");                        返回: "C:\\"
//   ExtractFilePath("/home/user1/data/test.c");   返回: "/home/user1/data/"
//-----------------------------------------------------------------------------
string ExtractFilePath(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif

	int nPos = GetLastDelimPos(strFileName, strDelims);
	return PathWithSlash(strFileName.substr(0, nPos + 1));
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出单独的文件名
// 参数:
//   strFileName - 包含路径的文件名
// 返回:
//   文件名
// 示例:
//   ExtractFileName("C:\\MyDir\\test.c");         返回: "test.c"
//   ExtractFilePath("/home/user1/data/test.c");   返回: "test.c"
//-----------------------------------------------------------------------------
string ExtractFileName(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif

	int nPos = GetLastDelimPos(strFileName, strDelims);
	return strFileName.substr(nPos + 1, strFileName.size() - nPos - 1);
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件扩展名
// 参数:
//   strFileName - 文件名 (可包含路径)
// 返回:
//   文件扩展名
// 示例:
//   ExtractFileExt("C:\\MyDir\\test.txt");         返回:  ".txt"
//   ExtractFileExt("/home/user1/data/test.c");     返回:  ".c"
//-----------------------------------------------------------------------------
string ExtractFileExt(const string& strFileName)
{
	string strDelims;
	strDelims += PATH_DELIM;
#ifdef ISE_WIN32
	strDelims += DRIVER_DELIM;
#endif
	strDelims += FILE_EXT_DELIM;

	int nPos = GetLastDelimPos(strFileName, strDelims);
	if (nPos >= 0 && strFileName[nPos] == FILE_EXT_DELIM)
		return strFileName.substr(nPos, strFileName.length());
	else
		return "";
}

//-----------------------------------------------------------------------------
// 描述: 改变文件名字符串中的文件扩展名
// 参数:
//   strFileName - 原文件名 (可包含路径)
//   strExt      - 新的文件扩展名
// 返回:
//   新的文件名
// 示例:
//   ChangeFileExt("c:\\test.txt", ".new");        返回:  "c:\\test.new"
//   ChangeFileExt("test.txt", ".new");            返回:  "test.new"
//   ChangeFileExt("test", ".new");                返回:  "test.new"
//   ChangeFileExt("test.txt", "");                返回:  "test"
//   ChangeFileExt("test.txt", ".");               返回:  "test."
//   ChangeFileExt("/home/user1/test.c", ".new");  返回:  "/home/user1/test.new"
//-----------------------------------------------------------------------------
string ChangeFileExt(const string& strFileName, const string& strExt)
{
	string strResult(strFileName);
	string strNewExt(strExt);

	if (!strResult.empty())
	{
		if (!strNewExt.empty() && strNewExt[0] != FILE_EXT_DELIM)
			strNewExt = FILE_EXT_DELIM + strNewExt;

		string strOldExt = ExtractFileExt(strResult);
		if (!strOldExt.empty())
			strResult.erase(strResult.length() - strOldExt.length());
		strResult += strNewExt;
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 强制创建目录
// 参数:
//   strDir - 待创建的目录 (可以是多级目录)
// 返回:
//   true   - 成功
//   false  - 失败
// 示例:
//   ForceDirectories("C:\\MyDir\\Test");
//   ForceDirectories("/home/user1/data");
//-----------------------------------------------------------------------------
bool ForceDirectories(string strDir)
{
	int nLen = (int)strDir.length();

	if (strDir.empty()) return false;
	if (strDir[nLen-1] == PATH_DELIM)
		strDir.resize(nLen - 1);

#ifdef ISE_WIN32
	if (strDir.length() < 3 || DirectoryExists(strDir) ||
		ExtractFilePath(strDir) == strDir) return true;    // avoid 'xyz:\' problem.
#endif
#ifdef ISE_LINUX
	if (strDir.empty() || DirectoryExists(strDir)) return true;
#endif
	return ForceDirectories(ExtractFilePath(strDir)) && CreateDir(strDir);
}

//-----------------------------------------------------------------------------
// 描述: 删除文件
//-----------------------------------------------------------------------------
bool DeleteFile(const string& strFileName)
{
#ifdef ISE_WIN32
	DWORD nFileAttr = ::GetFileAttributesA(strFileName.c_str());
	if (nFileAttr != (DWORD)(-1) && (nFileAttr & FILE_ATTRIBUTE_READONLY) != 0)
	{
		nFileAttr &= ~FILE_ATTRIBUTE_READONLY;
		::SetFileAttributesA(strFileName.c_str(), nFileAttr);
	}

	return ::DeleteFileA(strFileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
	return (unlink(strFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 同 DeleteFile()
//-----------------------------------------------------------------------------
bool RemoveFile(const string& strFileName)
{
	return DeleteFile(strFileName);
}

//-----------------------------------------------------------------------------
// 描述: 文件重命名
//-----------------------------------------------------------------------------
bool RenameFile(const string& strOldFileName, const string& strNewFileName)
{
#ifdef ISE_WIN32
	return ::MoveFileA(strOldFileName.c_str(), strNewFileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
	return (rename(strOldFileName.c_str(), strNewFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得文件的大小。若失败则返回-1
//-----------------------------------------------------------------------------
INT64 GetFileSize(const string& strFileName)
{
	INT64 nResult;

	try
	{
		CFileStream FileStream(strFileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
		nResult = FileStream.GetSize();
	}
	catch (CException&)
	{
		nResult = -1;
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 在指定路径下查找符合条件的文件
// 参数:
//   strPath    - 指定在哪个路径下进行查找，并必须指定通配符
//   nAttr      - 只查找符合此属性的文件
//   FindResult - 传回查找结果
// 示例:
//   FindFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   FindFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
//-----------------------------------------------------------------------------
void FindFiles(const string& strPath, UINT nAttr, FILE_FIND_RESULT& FindResult)
{
	const UINT FA_SPECIAL = FA_HIDDEN | FA_SYS_FILE | FA_VOLUME_ID | FA_DIRECTORY;
	UINT nExcludeAttr = ~nAttr & FA_SPECIAL;
	FindResult.clear();

#ifdef ISE_WIN32
	HANDLE nFindHandle;
	WIN32_FIND_DATAA FindData;

	nFindHandle = FindFirstFileA(strPath.c_str(), &FindData);
	if (nFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			DWORD nAttr = FindData.dwFileAttributes;
			string strName = FindData.cFileName;
			bool bIsSpecDir = (nAttr & FA_DIRECTORY) && (strName == "." || strName == "..");

			if ((nAttr & nExcludeAttr) == 0 && !bIsSpecDir)
			{
				FILE_FIND_ITEM Item;
				Item.nFileSize = FindData.nFileSizeHigh;
				Item.nFileSize = (Item.nFileSize << 32) | FindData.nFileSizeLow;
				Item.strFileName = strName;
				Item.nAttr = nAttr;

				FindResult.push_back(Item);
			}
		}
		while (FindNextFileA(nFindHandle, &FindData));

		FindClose(nFindHandle);
	}
#endif

#ifdef ISE_LINUX
	string strPathOnly = ExtractFilePath(strPath);
	string strPattern = ExtractFileName(strPath);
	string strFullName, strName;
	DIR *pDir;
	struct dirent DirEnt, *pDirEnt = NULL;
	struct stat StatBuf, LinkStatBuf;
	UINT nFileAttr, nFileMode;

	if (strPathOnly.empty()) strPathOnly = "/";

	pDir = opendir(strPathOnly.c_str());
	if (pDir)
	{
		while ((readdir_r(pDir, &DirEnt, &pDirEnt) == 0) && pDirEnt)
		{
			if (!fnmatch(strPattern.c_str(), pDirEnt->d_name, 0) == 0) continue;

			strName = pDirEnt->d_name;
			strFullName = strPathOnly + strName;

			if (lstat(strFullName.c_str(), &StatBuf) == 0)
			{
				nFileAttr = 0;
				nFileMode = StatBuf.st_mode;

				if (S_ISDIR(nFileMode))
					nFileAttr |= FA_DIRECTORY;
				else if (!S_ISREG(nFileMode))
				{
					if (S_ISLNK(nFileMode))
					{
						nFileAttr |= FA_SYM_LINK;
						if ((stat(strFullName.c_str(), &LinkStatBuf) == 0) &&
							(S_ISDIR(LinkStatBuf.st_mode)))
							nFileAttr |= FA_DIRECTORY;
					}
					nFileAttr |= FA_SYS_FILE;
				}

				if (pDirEnt->d_name[0] == '.' && pDirEnt->d_name[1])
					if (!(pDirEnt->d_name[1] == '.' && !pDirEnt->d_name[2]))
						nFileAttr |= FA_HIDDEN;

				if (euidaccess(strFullName.c_str(), W_OK) != 0)
					nFileAttr |= FA_READ_ONLY;

				bool bIsSpecDir = (nFileAttr & FA_DIRECTORY) && (strName == "." || strName == "..");

				if ((nFileAttr & nExcludeAttr) == 0 && !bIsSpecDir)
				{
					FILE_FIND_ITEM Item;
					Item.nFileSize = StatBuf.st_size;
					Item.strFileName = strName;
					Item.nAttr = nFileAttr;

					FindResult.push_back(Item);
				}
			}
		} // while

		closedir(pDir);
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 补全路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
string PathWithSlash(const string& strPath)
{
	string strResult = TrimString(strPath);
	int nLen = (int)strResult.size();
	if (nLen > 0 && strResult[nLen-1] != PATH_DELIM)
		strResult += PATH_DELIM;
	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 去掉路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
string PathWithoutSlash(const string& strPath)
{
	string strResult = TrimString(strPath);
	int nLen = (int)strResult.size();
	if (nLen > 0 && strResult[nLen-1] == PATH_DELIM)
		strResult.resize(nLen - 1);
	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件的全名(含绝对路径)
//-----------------------------------------------------------------------------
string GetAppExeName()
{
#ifdef ISE_WIN32
	char szBuffer[MAX_PATH];
	::GetModuleFileNameA(NULL, szBuffer, MAX_PATH);
	return string(szBuffer);
#endif
#ifdef ISE_LINUX
	const int BUFFER_SIZE = 1024;

	int r;
	char sBuffer[BUFFER_SIZE];
	string strResult;

	r = readlink("/proc/self/exe", sBuffer, BUFFER_SIZE);
	if (r != -1)
	{
		if (r >= BUFFER_SIZE) r = BUFFER_SIZE - 1;
		sBuffer[r] = 0;
		strResult = sBuffer;
	}
	else
	{
		IseThrowException(SEM_NO_PERM_READ_PROCSELFEXE);
	}

	return strResult;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
string GetAppPath()
{
	return ExtractFilePath(GetAppExeName());
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径的子目录
//-----------------------------------------------------------------------------
string GetAppSubPath(const string& strSubDir)
{
	return PathWithSlash(GetAppPath() + strSubDir);
}

//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码
//-----------------------------------------------------------------------------
int GetLastSysError()
{
#ifdef ISE_WIN32
	return ::GetLastError();
#endif
#ifdef ISE_LINUX
	return errno;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码对应的错误信息
//-----------------------------------------------------------------------------
string SysErrorMessage(int nErrorCode)
{
#ifdef ISE_WIN32
	char *pErrorMsg;

	pErrorMsg = strerror(nErrorCode);
	return pErrorMsg;
#endif
#ifdef ISE_LINUX
	const int ERROR_MSG_SIZE = 256;
	char sErrorMsg[ERROR_MSG_SIZE];
	string strResult;

	sErrorMsg[0] = 0;
	strerror_r(nErrorCode, sErrorMsg, ERROR_MSG_SIZE);
	if (sErrorMsg[0] == 0)
		strResult = FormatString("System error: %d", nErrorCode);
	else
		strResult = sErrorMsg;

	return strResult;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 睡眠 fSeconds 秒，可精确到纳秒。
// 参数:
//   fSeconds       - 睡眠的秒数，可为小数，可精确到纳秒 (实际精确度取决于操作系统)
//   AllowInterrupt - 是否允许信号中断
//-----------------------------------------------------------------------------
void SleepSec(double fSeconds, bool AllowInterrupt)
{
#ifdef ISE_WIN32
	Sleep((UINT)(fSeconds * 1000));
#endif
#ifdef ISE_LINUX
	const UINT NANO_PER_SEC = 1000000000;  // 一秒等于多少纳秒
	struct timespec req, remain;
	int r;

	req.tv_sec = (UINT)fSeconds;
	req.tv_nsec = (UINT)((fSeconds - req.tv_sec) * NANO_PER_SEC);

	while (true)
	{
		r = nanosleep(&req, &remain);
		if (r == -1 && errno == EINTR && !AllowInterrupt)
			req = remain;
		else
			break;
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得当前 Ticks，单位:毫秒
//-----------------------------------------------------------------------------
UINT GetCurTicks()
{
#ifdef ISE_WIN32
	return GetTickCount();
#endif
#ifdef ISE_LINUX
	timeval tv;
	gettimeofday(&tv, NULL);
	return INT64(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得两个 Ticks 之差
//-----------------------------------------------------------------------------
UINT GetTickDiff(UINT nOldTicks, UINT nNewTicks)
{
	if (nNewTicks >= nOldTicks)
		return (nNewTicks - nOldTicks);
	else
		return (UINT(-1) - nOldTicks + nNewTicks);
}

//-----------------------------------------------------------------------------
// 描述: 随机化 "随机数种子"
//-----------------------------------------------------------------------------
void Randomize()
{
	srand((unsigned int)time(NULL));
}

//-----------------------------------------------------------------------------
// 描述: 返回 [nMin..nMax] 之间的一个随机数，包含边界
//-----------------------------------------------------------------------------
int GetRandom(int nMin, int nMax)
{
	ISE_ASSERT((nMax - nMin) < MAXLONG);
	return nMin + (int)(((double)rand() / ((double)RAND_MAX + 0.1)) * (nMax - nMin + 1));
}

//-----------------------------------------------------------------------------
// 描述: 生成一组介于区间 [nStartNumber, nEndNumber] 内的不重复的随机数
// 注意: 数组 pRandomList 的容量必须 >= (nEndNumber - nStartNumber + 1)
//-----------------------------------------------------------------------------
void GenerateRandomList(int nStartNumber, int nEndNumber, int *pRandomList)
{
	if (nStartNumber > nEndNumber || !pRandomList) return;

	int nCount = nEndNumber - nStartNumber + 1;

	if (rand() % 2 == 0)
		for (int i = 0; i < nCount; i++)
			pRandomList[i] = nStartNumber + i;
	else
		for (int i = 0; i < nCount; i++)
			pRandomList[nCount - i - 1] = nStartNumber + i;

	for (int i = nCount - 1; i >= 1; i--)
		Swap(pRandomList[i], pRandomList[rand()%i]);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
