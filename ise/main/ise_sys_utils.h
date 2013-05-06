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
// ise_sys_utils.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SYS_UTILS_H_
#define _ISE_SYS_UTILS_H_

#include "ise/main/ise_options.h"

#ifdef ISE_WINDOWS
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
#include <sys/syscall.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#endif

#include "ise/main/ise_global_defs.h"

using namespace std;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class Buffer;
class StrList;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 文件查找记录
struct FileFindItem
{
    INT64 fileSize;         // 文件大小
    string fileName;        // 文件名(不含路径)
    UINT attr;              // 文件属性
};

typedef vector<FileFindItem> FileFindResult;

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
//-- 字符串函数:

bool isIntStr(const string& str);
bool isInt64Str(const string& str);
bool isFloatStr(const string& str);
bool isBoolStr(const string& str);

int strToInt(const string& str, int defaultVal = 0);
INT64 strToInt64(const string& str, INT64 defaultVal = 0);
string intToStr(int value);
string intToStr(INT64 value);
double strToFloat(const string& str, double defaultVal = 0);
string floatToStr(double value, const char *format = "%f");
bool strToBool(const string& str, bool defaultVal = false);
string boolToStr(bool value, bool useBoolStrs = false);

void formatStringV(string& result, const char *format, va_list argList);
string formatString(const char *format, ...);

bool sameText(const string& str1, const string& str2);
int compareText(const char* str1, const char* str2);
string trimString(const string& str);
string upperCase(const string& str);
string lowerCase(const string& str);
string repalceString(const string& sourceStr, const string& oldPattern,
    const string& newPattern, bool replaceAll = false, bool caseSensitive = true);
void splitString(const string& sourceStr, char splitter, StrList& strList,
    bool trimResult = false);
void splitStringToInt(const string& sourceStr, char splitter, IntegerArray& intList);
char* strNCopy(char *dest, const char *source, int maxBytes);
char* strNZCopy(char *dest, const char *source, int destSize);
string fetchStr(string& inputStr, char delimiter = ' ', bool del = true);
string addThousandSep(const INT64& number);

string getQuotedStr(const char* str, char quoteChar = '"');
string extractQuotedStr(const char*& str, char quoteChar = '"');
string getDequotedStr(const char* str, char quoteChar = '"');

//-----------------------------------------------------------------------------
//-- 文件和目录:

bool fileExists(const string& fileName);
bool directoryExists(const string& dir);
bool createDir(const string& dir);
bool deleteDir(const string& dir, bool recursive = false);
string extractFilePath(const string& fileName);
string extractFileName(const string& fileName);
string extractFileExt(const string& fileName);
string changeFileExt(const string& fileName, const string& ext);
bool forceDirectories(string dir);
bool deleteFile(const string& fileName);
bool removeFile(const string& fileName);
bool renameFile(const string& oldFileName, const string& newFileName);
INT64 getFileSize(const string& fileName);
void findFiles(const string& path, UINT attr, FileFindResult& findResult);
string pathWithSlash(const string& path);
string pathWithoutSlash(const string& path);
string GetAppExeName();
string getAppPath();
string getAppSubPath(const string& subDir = "");

//-----------------------------------------------------------------------------
//-- 系统相关:

int getLastSysError();
THREAD_ID getCurThreadId();
string sysErrorMessage(int errorCode);
void sleepSec(double seconds, bool allowInterrupt = true);
UINT getCurTicks();
UINT getTickDiff(UINT oldTicks, UINT newTicks);

//-----------------------------------------------------------------------------
//-- 其它函数:

void randomize();
int getRandom(int min, int max);
void generateRandomList(int startNumber, int endNumber, int *randomList);

template <typename T>
const T& min(const T& a, const T& b) { return ((a < b)? a : b); }

template <typename T>
const T& max(const T& a, const T& b) { return ((a < b)? b : a); }

template <typename T>
const T& ensureRange(const T& value, const T& minVal, const T& maxVal)
    { return (value > maxVal) ? maxVal : (value < minVal ? minVal : value); }

template <typename T>
void swap(T& a, T& b) { T temp(a); a=b; b=temp; }

template <typename T>
int compare(const T& a, const T& b) { return (a < b) ? -1 : (a > b ? 1 : 0); }

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SYS_UTILS_H_
