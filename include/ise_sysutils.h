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
// ise_sysutils.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SYSUTILS_H_
#define _ISE_SYSUTILS_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <windows.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#endif

#include "ise_global_defs.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CBuffer;
class CStrList;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 文件查找记录
struct FILE_FIND_ITEM
{
	INT64 nFileSize;         // 文件大小
	string strFileName;      // 文件名(不含路径)
	UINT nAttr;              // 文件属性
};

typedef vector<FILE_FIND_ITEM> FILE_FIND_RESULT;

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
//-- 字符串函数:

bool IsIntStr(const string& str);
bool IsInt64Str(const string& str);
bool IsFloatStr(const string& str);
bool IsBoolStr(const string& str);

int StrToInt(const string& str, int nDefault = 0);
INT64 StrToInt64(const string& str, INT64 nDefault = 0);
string IntToStr(int nValue);
string IntToStr(INT64 nValue);
double StrToFloat(const string& str, double fDefault = 0);
string FloatToStr(double fValue, const char *sFormat = "%f");
bool StrToBool(const string& str, bool bDefault = false);
string BoolToStr(bool bValue, bool bUseBoolStrs = false);

void FormatStringV(string& strResult, const char *sFormatString, va_list argList);
string FormatString(const char *sFormatString, ...);

bool SameText(const string& str1, const string& str2);
int CompareText(const char* str1, const char* str2);
string TrimString(const string& str);
string UpperCase(const string& str);
string LowerCase(const string& str);
string RepalceString(const string& strSource, const string& strOldPattern,
	const string& strNewPattern, bool bReplaceAll = false, bool bCaseSensitive = true);
void SplitString(const string& strSource, char chSplitter, CStrList& StrList,
	bool bTrimResult = false);
void SplitStringToInt(const string& strSource, char chSplitter, IntegerArray& IntList);
char* StrNCopy(char *pDest, const char *pSource, int nMaxBytes);
char* StrNZCopy(char *pDest, const char *pSource, int nDestSize);
string FetchStr(string& strInput, char chDelimiter = ' ', bool bDelete = true);
string AddThousandSep(const INT64& nNumber);

string GetQuotedStr(const char* lpszStr, char chQuote = '"');
string ExtractQuotedStr(const char*& lpszStr, char chQuote = '"');
string GetDequotedStr(const char* lpszStr, char chQuote = '"');

//-----------------------------------------------------------------------------
//-- 文件和目录:

bool FileExists(const string& strFileName);
bool DirectoryExists(const string& strDir);
bool CreateDir(const string& strDir);
bool DeleteDir(const string& strDir, bool bRecursive = false);
string ExtractFilePath(const string& strFileName);
string ExtractFileName(const string& strFileName);
string ExtractFileExt(const string& strFileName);
string ChangeFileExt(const string& strFileName, const string& strExt);
bool ForceDirectories(string strDir);
bool DeleteFile(const string& strFileName);
bool RemoveFile(const string& strFileName);
bool RenameFile(const string& strOldFileName, const string& strNewFileName);
INT64 GetFileSize(const string& strFileName);
void FindFiles(const string& strPath, UINT nAttr, FILE_FIND_RESULT& FindResult);
string PathWithSlash(const string& strPath);
string PathWithoutSlash(const string& strPath);
string GetAppExeName();
string GetAppPath();
string GetAppSubPath(const string& strSubDir = "");

//-----------------------------------------------------------------------------
//-- 系统相关:

int GetLastSysError();
string SysErrorMessage(int nErrorCode);
void SleepSec(double fSeconds, bool AllowInterrupt = true);
UINT GetCurTicks();
UINT GetTickDiff(UINT nOldTicks, UINT nNewTicks);

//-----------------------------------------------------------------------------
//-- 其它函数:

void Randomize();
int GetRandom(int nMin, int nMax);
void GenerateRandomList(int nStartNumber, int nEndNumber, int *pRandomList);

template <typename T>
inline const T& Min(const T& a, const T& b) { return ((a < b)? a : b); }

template <typename T>
inline const T& Max(const T& a, const T& b) { return ((a < b)? b : a); }

template <typename T>
inline const T& EnsureRange(const T& Value, const T& Min, const T& Max)
	{ return (Value > Max) ? Max : (Value < Min ? Min : Value); }

template <typename T>
inline void Swap(T& v1, T& v2)
	{ T temp; temp = v1; v1 = v2; v2 = temp; }

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SYSUTILS_H_
