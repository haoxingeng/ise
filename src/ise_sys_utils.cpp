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
// 文件名称: ise_sys_utils.cpp
// 功能描述: 系统杂项功能
///////////////////////////////////////////////////////////////////////////////

#include "ise_sys_utils.h"
#include "ise_classes.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个整数
//-----------------------------------------------------------------------------
bool isIntStr(const string& str)
{
    bool result;
    int len = (int)str.size();
    char *strPtr = (char*)str.c_str();

    result = (len > 0) && !isspace(strPtr[0]);

    if (result)
    {
        char *endp;
        strtol(strPtr, &endp, 10);
        result = (endp - strPtr == len);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个整数
//-----------------------------------------------------------------------------
bool isInt64Str(const string& str)
{
    bool result;
    int len = (int)str.size();
    char *strPtr = (char*)str.c_str();

    result = (len > 0) && !isspace(strPtr[0]);

    if (result)
    {
        char *endp;
#ifdef ISE_WINDOWS
        _strtoi64(strPtr, &endp, 10);
#endif
#ifdef ISE_LINUX
        strtoll(strPtr, &endp, 10);
#endif
        result = (endp - strPtr == len);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个浮点数
//-----------------------------------------------------------------------------
bool isFloatStr(const string& str)
{
    bool result;
    int len = (int)str.size();
    char *strPtr = (char*)str.c_str();

    result = (len > 0) && !isspace(strPtr[0]);

    if (result)
    {
        char *endp;
        strtod(strPtr, &endp);
        result = (endp - strPtr == len);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串可否转换成布尔型
//-----------------------------------------------------------------------------
bool isBoolStr(const string& str)
{
    return sameText(str, TRUE_STR) || sameText(str, FALSE_STR) || isFloatStr(str);
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成整型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
int strToInt(const string& str, int defaultVal)
{
    if (isIntStr(str))
        return strtol(str.c_str(), NULL, 10);
    else
        return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成64位整型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
INT64 strToInt64(const string& str, INT64 defaultVal)
{
    if (isInt64Str(str))
#ifdef ISE_WINDOWS
        return _strtoi64(str.c_str(), NULL, 10);
#endif
#ifdef ISE_LINUX
        return strtol(str.c_str(), NULL, 10);
#endif
    else
        return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 整型转换成字符串
//-----------------------------------------------------------------------------
string intToStr(int value)
{
    char temp[64];
    sprintf(temp, "%d", value);
    return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 64位整型转换成字符串
//-----------------------------------------------------------------------------
string intToStr(INT64 value)
{
    char temp[64];
#ifdef ISE_WINDOWS
    sprintf(temp, "%I64d", value);
#endif
#ifdef ISE_LINUX
    sprintf(temp, "%lld", value);
#endif
    return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成浮点型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
double strToFloat(const string& str, double defaultVal)
{
    if (isFloatStr(str))
        return strtod(str.c_str(), NULL);
    else
        return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 浮点型转换成字符串
//-----------------------------------------------------------------------------
string floatToStr(double value, const char *format)
{
    char temp[256];
    sprintf(temp, format, value);
    return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成布尔型
//-----------------------------------------------------------------------------
bool strToBool(const string& str, bool defaultVal)
{
    if (isBoolStr(str))
    {
        if (sameText(str, TRUE_STR))
            return true;
        else if (sameText(str, FALSE_STR))
            return false;
        else
            return (strToFloat(str, 0) != 0);
    }
    else
        return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 布尔型转换成字符串
//-----------------------------------------------------------------------------
string boolToStr(bool value, bool useBoolStrs)
{
    if (useBoolStrs)
        return (value? TRUE_STR : FALSE_STR);
    else
        return (value? "1" : "0");
}

//-----------------------------------------------------------------------------
// 描述: 格式化字符串 (供formatString函数调用)
//-----------------------------------------------------------------------------
void formatStringV(string& result, const char *format, va_list argList)
{
#if defined(ISE_COMPILER_VC)

    int size = _vscprintf(format, argList);
    char *buffer = (char *)malloc(size + 1);
    if (buffer)
    {
        vsprintf(buffer, format, argList);
        result = buffer;
        free(buffer);
    }
    else
        result = "";

#else

    int size = 100;
    char *buffer = (char *)malloc(size);
    va_list args;

    while (buffer)
    {
        int charCount;

        va_copy(args, argList);
        charCount = vsnprintf(buffer, size, format, args);
        va_end(args);

        if (charCount > -1 && charCount < size)
            break;
        if (charCount > -1)
            size = charCount + 1;
        else
            size *= 2;
        buffer = (char *)realloc(buffer, size);
    }

    if (buffer)
    {
        result = buffer;
        free(buffer);
    }
    else
        result = "";

#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回格式化后的字符串
// 参数:
//   format  - 格式化字符串
//   ...            - 格式化参数
//-----------------------------------------------------------------------------
string formatString(const char *format, ...)
{
    string result;

    va_list argList;
    va_start(argList, format);
    formatStringV(result, format, argList);
    va_end(argList);

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断两个字符串是否相同 (不区分大小写)
//-----------------------------------------------------------------------------
bool sameText(const string& str1, const string& str2)
{
    return compareText(str1.c_str(), str2.c_str()) == 0;
}

//-----------------------------------------------------------------------------
// 描述: 比较两个字符串 (不区分大小写)
//-----------------------------------------------------------------------------
int compareText(const char* str1, const char* str2)
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
string trimString(const string& str)
{
    string result;
    int i, len;

    len = (int)str.size();
    i = 0;
    while (i < len && (BYTE)str[i] <= 32) i++;
    if (i < len)
    {
        while ((BYTE)str[len-1] <= 32) len--;
        result = str.substr(i, len - i);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变大写
//-----------------------------------------------------------------------------
string upperCase(const string& str)
{
    string result = str;
    int len = (int)result.size();
    char c;

    for (int i = 0; i < len; i++)
    {
        c = result[i];
        if (c >= 'a' && c <= 'z')
            result[i] = c - 32;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变小写
//-----------------------------------------------------------------------------
string lowerCase(const string& str)
{
    string result = str;
    int len = (int)result.size();
    char c;

    for (int i = 0; i < len; i++)
    {
        c = result[i];
        if (c >= 'A' && c <= 'Z')
            result[i] = c + 32;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串替换
// 参数:
//   sourceStr        - 源串
//   oldPattern    - 源串中将被替换的字符串
//   newPattern    - 取代 oldPattern 的字符串
//   replaceAll      - 是否替换源串中所有匹配的字符串(若为false，则只替换第一处)
//   caseSensitive   - 是否区分大小写
// 返回:
//   进行替换动作之后的字符串
//-----------------------------------------------------------------------------
string repalceString(const string& sourceStr, const string& oldPattern,
    const string& newPattern, bool replaceAll, bool caseSensitive)
{
    string result = sourceStr;
    string searchStr, patternStr;
    string::size_type offset, index;
    int oldPattLen, newPattLen;

    if (!caseSensitive)
    {
        searchStr = upperCase(sourceStr);
        patternStr = upperCase(oldPattern);
    }
    else
    {
        searchStr = sourceStr;
        patternStr = oldPattern;
    }

    oldPattLen = (int)oldPattern.size();
    newPattLen = (int)newPattern.size();
    index = 0;

    while (index < searchStr.size())
    {
        offset = searchStr.find(patternStr, index);
        if (offset == string::npos) break;  // 若没找到

        searchStr.replace(offset, oldPattLen, newPattern);
        result.replace(offset, oldPattLen, newPattern);
        index = (offset + newPattLen);

        if (!replaceAll) break;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 分割字符串
// 参数:
//   sourceStr   - 源串
//   splitter  - 分隔符
//   strList     - 存放分割之后的字符串列表
//   trimResult - 是否对分割后的结果进行 trim 处理
// 示例:
//   ""          -> []
//   " "         -> [" "]
//   ","         -> ["", ""]
//   "a,b,c"     -> ["a", "b", "c"]
//   ",a,,b,c,"  -> ["", "a", "", "b", "c", ""]
//-----------------------------------------------------------------------------
void splitString(const string& sourceStr, char splitter, StrList& strList,
    bool trimResult)
{
    string::size_type offset, index = 0;

    strList.clear();
    if (sourceStr.empty()) return;

    while (true)
    {
        offset = sourceStr.find(splitter, index);
        if (offset == string::npos)   // 若没找到
        {
            strList.add(sourceStr.substr(index).c_str());
            break;
        }
        else
        {
            strList.add(sourceStr.substr(index, offset - index).c_str());
            index = offset + 1;
        }
    }

    if (trimResult)
    {
        for (int i = 0; i < strList.getCount(); i++)
            strList.setString(i, trimString(strList[i]).c_str());
    }
}

//-----------------------------------------------------------------------------
// 描述: 分割字符串并转换成整型数列表
// 参数:
//   sourceStr  - 源串
//   splitter - 分隔符
//   intList    - 存放分割之后的整型数列表
//-----------------------------------------------------------------------------
void splitStringToInt(const string& sourceStr, char splitter, IntegerArray& intList)
{
    StrList strList;
    splitString(sourceStr, splitter, strList, true);

    intList.clear();
    for (int i = 0; i < strList.getCount(); i++)
        intList.push_back(atoi(strList[i].c_str()));
}

//-----------------------------------------------------------------------------
// 描述: 复制串 source 到 dest 中
// 备注:
//   1. 最多只复制 maxBytes 个字节到 dest 中，包括结束符'\0'。
//   2. 如果 source 的实际长度(strlen)小于 maxBytes，则复制会提前结束，
//      dest 的剩余部分以 '\0' 填充。
//   3. 如果 source 的实际长度(strlen)大于 maxBytes，则复制之后的 dest 没有结束符。
//-----------------------------------------------------------------------------
char *strNCopy(char *dest, const char *source, int maxBytes)
{
    if (maxBytes > 0)
    {
        if (source)
            return strncpy(dest, source, maxBytes);
        else
            return strcpy(dest, "");
    }

    return dest;
}

//-----------------------------------------------------------------------------
// 描述: 复制串 source 到 dest 中
// 备注: 最多只复制 destSize 个字节到 dest 中。并将 dest 的最后字节设为'\0'。
// 参数:
//   destSize - dest的大小
//-----------------------------------------------------------------------------
char *strNZCopy(char *dest, const char *source, int destSize)
{
    if (destSize > 0)
    {
        if (source)
        {
            char *p;
            p = strncpy(dest, source, destSize);
            dest[destSize - 1] = '\0';
            return p;
        }
        else
            return strcpy(dest, "");
    }
    else
        return dest;
}

//-----------------------------------------------------------------------------
// 描述: 从源串中获取一个子串
//
// For example:
//   inputStr(before)   delimiter  del       inputStr(after)   result(after)
//   ----------------   -----------  ----------    ---------------   -------------
//   "abc def"           ' '         false         "abc def"         "abc"
//   "abc def"           ' '         true          "def"             "abc"
//   " abc"              ' '         false         " abc"            ""
//   " abc"              ' '         true          "abc"             ""
//   ""                  ' '         true/false    ""                ""
//-----------------------------------------------------------------------------
string fetchStr(string& inputStr, char delimiter, bool del)
{
    string result;

    string::size_type pos = inputStr.find(delimiter, 0);
    if (pos == string::npos)
    {
        result = inputStr;
        if (del)
            inputStr.clear();
    }
    else
    {
        result = inputStr.substr(0, pos);
        if (del)
            inputStr = inputStr.substr(pos + 1);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 在数字中间插入逗号进行数据分组
//-----------------------------------------------------------------------------
string addThousandSep(const INT64& number)
{
    string result = intToStr(number);
    for (int i = (int)result.length() - 3; i > 0; i -= 3)
        result.insert(i, ",");
    return result;
}

//-----------------------------------------------------------------------------
// Converts a string to a quoted string.
// For example:
//    abc         ->     "abc"
//    ab'c        ->     "ab'c"
//    ab"c        ->     "ab""c"
//    (empty)     ->     ""
//-----------------------------------------------------------------------------
string getQuotedStr(const char* str, char quoteChar)
{
    string result;
    string srcStr(str);

    result = quoteChar;

    string::size_type start = 0;
    while (true)
    {
        string::size_type pos = srcStr.find(quoteChar, start);
        if (pos != string::npos)
        {
            result += srcStr.substr(start, pos - start) + quoteChar + quoteChar;
            start = pos + 1;
        }
        else
        {
            result += srcStr.substr(start);
            break;
        }
    }

    result += quoteChar;

    return result;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// extractQuotedStr removes the quote characters from the beginning and end of a quoted string,
// and reduces pairs of quote characters within the string to a single quote character.
// The @a quoteChar parameter defines what character to use as a quote character. If the first
// character in @a str is not the value of the @a quoteChar parameter, extractQuotedStr returns
// an empty string.
//
// The function copies characters from @a str to the result string until the second solitary
// quote character or the first null character in @a str. The @a str parameter is updated
// to point to the first character following the quoted string. If @a str does not contain a
// matching end quote character, the @a str parameter is updated to point to the terminating
// null character.
//
// For example:
//    str(before)    Returned string        str(after)
//    ---------------    ---------------        ---------------
//    "abc"               abc                    '\0'
//    "ab""c"             ab"c                   '\0'
//    "abc"123            abc                    123
//    abc"                (empty)                abc"
//    "abc                abc                    '\0'
//    (empty)             (empty)                '\0'
//-----------------------------------------------------------------------------
string extractQuotedStr(const char*& str, char quoteChar)
{
    string result;
    const char* startPtr = str;

    if (str == NULL || *str != quoteChar)
        return "";

    // Calc the character count after converting.

    int size = 0;
    str++;
    while (*str != '\0')
    {
        if (str[0] == quoteChar)
        {
            if (str[1] == quoteChar)
            {
                size++;
                str += 2;
            }
            else
            {
                str++;
                break;
            }
        }
        else
        {
            const char* p = str;
            str++;
            size += (int)(str - p);
        }
    }

    // Begin to retrieve the characters.

    result.resize(size);
    char* resultPtr = (char*)result.c_str();
    str = startPtr;
    str++;
    while (*str != '\0')
    {
        if (str[0] == quoteChar)
        {
            if (str[1] == quoteChar)
            {
                *resultPtr++ = *str;
                str += 2;
            }
            else
            {
                str++;
                break;
            }
        }
        else
        {
            const char* p = str;
            str++;
            while (p < str)
                *resultPtr++ = *p++;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// getDequotedStr removes the quote characters from the beginning and end of a quoted string, and
// reduces pairs of quote characters within the string to a single quote character. The quoteChar
// parameter defines what character to use as a quote character. If the @a str parameter does
// not begin and end with the quote character, getDequotedStr returns @a str unchanged.
//
// For example:
//    "abc"     ->     abc
//    "ab""c"   ->     ab"c
//    "abc      ->     "abc
//    abc"      ->     abc"
//    (empty)   ->    (empty)
//-----------------------------------------------------------------------------
string getDequotedStr(const char* str, char quoteChar)
{
    const char* startPtr = str;
    int strLen = (int)strlen(str);

    string result = extractQuotedStr(str, quoteChar);

    if ( (result.empty() || *str == '\0') &&
        strLen > 0 && (startPtr[0] != quoteChar || startPtr[strLen-1] != quoteChar) )
        result = startPtr;

    return result;
}

//-----------------------------------------------------------------------------

#ifdef ISE_WINDOWS
static bool GetFileFindData(const string& fileName, WIN32_FIND_DATAA& findData)
{
    HANDLE findHandle = ::FindFirstFileA(fileName.c_str(), &findData);
    bool result = (findHandle != INVALID_HANDLE_VALUE);
    if (result) ::FindClose(findHandle);
    return result;
}
#endif

//-----------------------------------------------------------------------------
// 描述: 检查文件是否存在
//-----------------------------------------------------------------------------
bool fileExists(const string& fileName)
{
#ifdef ISE_WINDOWS
    DWORD fileAttr = ::GetFileAttributesA(fileName.c_str());
    if (fileAttr != (DWORD)(-1))
        return ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
    else
    {
        WIN32_FIND_DATAA findData;
        DWORD lastError = ::GetLastError();
        return
            (lastError != ERROR_FILE_NOT_FOUND) &&
            (lastError != ERROR_PATH_NOT_FOUND) &&
            (lastError != ERROR_INVALID_NAME) &&
            GetFileFindData(fileName, findData) &&
            (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }
#endif
#ifdef ISE_LINUX
    return (euidaccess(fileName.c_str(), F_OK) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 检查目录是否存在
//-----------------------------------------------------------------------------
bool directoryExists(const string& dir)
{
#ifdef ISE_WINDOWS
    int code;
    code = GetFileAttributesA(dir.c_str());
    return (code != -1) && ((FILE_ATTRIBUTE_DIRECTORY & code) != 0);
#endif
#ifdef ISE_LINUX
    string path = pathWithSlash(dir);
    struct stat st;
    bool result;

    if (stat(path.c_str(), &st) == 0)
        result = ((st.st_mode & S_IFDIR) == S_IFDIR);
    else
        result = false;

    return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 创建目录
// 示例:
//   createDir("C:\\test");
//   createDir("/home/test");
//-----------------------------------------------------------------------------
bool createDir(const string& dir)
{
#ifdef ISE_WINDOWS
    return CreateDirectoryA(dir.c_str(), NULL) != 0;
#endif
#ifdef ISE_LINUX
    return mkdir(dir.c_str(), (mode_t)(-1)) == 0;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 删除目录
// 参数:
//   dir     - 待删除的目录
//   recursive - 是否递归删除
// 返回:
//   true   - 成功
//   false  - 失败
//-----------------------------------------------------------------------------
bool deleteDir(const string& dir, bool recursive)
{
    if (!recursive)
    {
#ifdef ISE_WINDOWS
        return RemoveDirectoryA(dir.c_str()) != 0;
#endif
#ifdef ISE_LINUX
        return rmdir(dir.c_str()) == 0;
#endif
    }

#ifdef ISE_WINDOWS
    const char* const ALL_FILE_WILDCHAR = "*.*";
#endif
#ifdef ISE_LINUX
    const char* const ALL_FILE_WILDCHAR = "*";
#endif

    bool result = true;
    string path = pathWithSlash(dir);
    FileFindResult fr;
    findFiles(path + ALL_FILE_WILDCHAR, FA_ANY_FILE, fr);

    for (int i = 0; i < (int)fr.size() && result; i++)
    {
        string fullName = path + fr[i].fileName;
        if (fr[i].attr & FA_DIRECTORY)
            result = deleteDir(fullName, true);
        else
            removeFile(fullName);
    }

    result = deleteDir(path, false);

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得文件名中最后一个分隔符的位置(0-based)。若没有，则返回-1
//-----------------------------------------------------------------------------
int getLastDelimPos(const string& fileName, const string& delims)
{
    int result = (int)fileName.size() - 1;

    for (; result >= 0; result--)
        if (delims.find(fileName[result], 0) != string::npos)
            break;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件路径
// 参数:
//   fileName - 包含路径的文件名
// 返回:
//   文件的路径
// 示例:
//   extractFilePath("C:\\MyDir\\test.c");         返回: "C:\\MyDir\\"
//   extractFilePath("C:");                        返回: "C:\\"
//   extractFilePath("/home/user1/data/test.c");   返回: "/home/user1/data/"
//-----------------------------------------------------------------------------
string extractFilePath(const string& fileName)
{
    string delims;
    delims += PATH_DELIM;
#ifdef ISE_WINDOWS
    delims += DRIVER_DELIM;
#endif

    int pos = getLastDelimPos(fileName, delims);
    return pathWithSlash(fileName.substr(0, pos + 1));
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出单独的文件名
// 参数:
//   fileName - 包含路径的文件名
// 返回:
//   文件名
// 示例:
//   extractFileName("C:\\MyDir\\test.c");         返回: "test.c"
//   extractFilePath("/home/user1/data/test.c");   返回: "test.c"
//-----------------------------------------------------------------------------
string extractFileName(const string& fileName)
{
    string delims;
    delims += PATH_DELIM;
#ifdef ISE_WINDOWS
    delims += DRIVER_DELIM;
#endif

    int pos = getLastDelimPos(fileName, delims);
    return fileName.substr(pos + 1, fileName.size() - pos - 1);
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件扩展名
// 参数:
//   fileName - 文件名 (可包含路径)
// 返回:
//   文件扩展名
// 示例:
//   extractFileExt("C:\\MyDir\\test.txt");         返回:  ".txt"
//   extractFileExt("/home/user1/data/test.c");     返回:  ".c"
//-----------------------------------------------------------------------------
string extractFileExt(const string& fileName)
{
    string delims;
    delims += PATH_DELIM;
#ifdef ISE_WINDOWS
    delims += DRIVER_DELIM;
#endif
    delims += FILE_EXT_DELIM;

    int pos = getLastDelimPos(fileName, delims);
    if (pos >= 0 && fileName[pos] == FILE_EXT_DELIM)
        return fileName.substr(pos, fileName.length());
    else
        return "";
}

//-----------------------------------------------------------------------------
// 描述: 改变文件名字符串中的文件扩展名
// 参数:
//   fileName - 原文件名 (可包含路径)
//   ext      - 新的文件扩展名
// 返回:
//   新的文件名
// 示例:
//   changeFileExt("c:\\test.txt", ".new");        返回:  "c:\\test.new"
//   changeFileExt("test.txt", ".new");            返回:  "test.new"
//   changeFileExt("test", ".new");                返回:  "test.new"
//   changeFileExt("test.txt", "");                返回:  "test"
//   changeFileExt("test.txt", ".");               返回:  "test."
//   changeFileExt("/home/user1/test.c", ".new");  返回:  "/home/user1/test.new"
//-----------------------------------------------------------------------------
string changeFileExt(const string& fileName, const string& ext)
{
    string result(fileName);
    string newExt(ext);

    if (!result.empty())
    {
        if (!newExt.empty() && newExt[0] != FILE_EXT_DELIM)
            newExt = FILE_EXT_DELIM + newExt;

        string oldExt = extractFileExt(result);
        if (!oldExt.empty())
            result.erase(result.length() - oldExt.length());
        result += newExt;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 强制创建目录
// 参数:
//   dir - 待创建的目录 (可以是多级目录)
// 返回:
//   true   - 成功
//   false  - 失败
// 示例:
//   forceDirectories("C:\\MyDir\\Test");
//   forceDirectories("/home/user1/data");
//-----------------------------------------------------------------------------
bool forceDirectories(string dir)
{
    int len = (int)dir.length();

    if (dir.empty()) return false;
    if (dir[len-1] == PATH_DELIM)
        dir.resize(len - 1);

#ifdef ISE_WINDOWS
    if (dir.length() < 3 || directoryExists(dir) ||
        extractFilePath(dir) == dir) return true;    // avoid 'xyz:\' problem.
#endif
#ifdef ISE_LINUX
    if (dir.empty() || directoryExists(dir)) return true;
#endif
    return forceDirectories(extractFilePath(dir)) && createDir(dir);
}

//-----------------------------------------------------------------------------
// 描述: 删除文件
//-----------------------------------------------------------------------------
bool deleteFile(const string& fileName)
{
#ifdef ISE_WINDOWS
    DWORD fileAttr = ::GetFileAttributesA(fileName.c_str());
    if (fileAttr != (DWORD)(-1) && (fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
    {
        fileAttr &= ~FILE_ATTRIBUTE_READONLY;
        ::SetFileAttributesA(fileName.c_str(), fileAttr);
    }

    return ::DeleteFileA(fileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
    return (unlink(fileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 同 deleteFile()
//-----------------------------------------------------------------------------
bool removeFile(const string& fileName)
{
    return deleteFile(fileName);
}

//-----------------------------------------------------------------------------
// 描述: 文件重命名
//-----------------------------------------------------------------------------
bool renameFile(const string& oldFileName, const string& newFileName)
{
#ifdef ISE_WINDOWS
    return ::MoveFileA(oldFileName.c_str(), newFileName.c_str()) != 0;
#endif
#ifdef ISE_LINUX
    return (rename(oldFileName.c_str(), newFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得文件的大小。若失败则返回-1
//-----------------------------------------------------------------------------
INT64 getFileSize(const string& fileName)
{
    INT64 result;

    try
    {
        FileStream fileStream(fileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
        result = fileStream.getSize();
    }
    catch (Exception&)
    {
        result = -1;
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 在指定路径下查找符合条件的文件
// 参数:
//   path    - 指定在哪个路径下进行查找，并必须指定通配符
//   attr      - 只查找符合此属性的文件
//   findResult - 传回查找结果
// 示例:
//   findFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   findFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
//-----------------------------------------------------------------------------
void findFiles(const string& path, UINT attr, FileFindResult& findResult)
{
    const UINT FA_SPECIAL = FA_HIDDEN | FA_SYS_FILE | FA_VOLUME_ID | FA_DIRECTORY;
    UINT excludeAttr = ~attr & FA_SPECIAL;
    findResult.clear();

#ifdef ISE_WINDOWS
    HANDLE findHandle;
    WIN32_FIND_DATAA findData;

    findHandle = FindFirstFileA(path.c_str(), &findData);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD attr = findData.dwFileAttributes;
            string name = findData.cFileName;
            bool isSpecDir = (attr & FA_DIRECTORY) && (name == "." || name == "..");

            if ((attr & excludeAttr) == 0 && !isSpecDir)
            {
                FileFindItem item;
                item.fileSize = findData.nFileSizeHigh;
                item.fileSize = (item.fileSize << 32) | findData.nFileSizeLow;
                item.fileName = name;
                item.attr = attr;

                findResult.push_back(item);
            }
        }
        while (FindNextFileA(findHandle, &findData));

        FindClose(findHandle);
    }
#endif

#ifdef ISE_LINUX
    string pathOnlyStr = extractFilePath(path);
    string patternStr = extractFileName(path);
    string fullName, name;
    DIR *dirPtr;
    struct dirent dirEnt, *dirEntPtr = NULL;
    struct stat statBuf, linkStatBuf;
    UINT fileAttr, fileMode;

    if (pathOnlyStr.empty()) pathOnlyStr = "/";

    dirPtr = opendir(pathOnlyStr.c_str());
    if (dirPtr)
    {
        while ((readdir_r(dirPtr, &dirEnt, &dirEntPtr) == 0) && dirEntPtr)
        {
            if (!fnmatch(patternStr.c_str(), dirEntPtr->d_name, 0) == 0) continue;

            name = dirEntPtr->d_name;
            fullName = pathOnlyStr + name;

            if (lstat(fullName.c_str(), &statBuf) == 0)
            {
                fileAttr = 0;
                fileMode = statBuf.st_mode;

                if (S_ISDIR(fileMode))
                    fileAttr |= FA_DIRECTORY;
                else if (!S_ISREG(fileMode))
                {
                    if (S_ISLNK(fileMode))
                    {
                        fileAttr |= FA_SYM_LINK;
                        if ((stat(fullName.c_str(), &linkStatBuf) == 0) &&
                            (S_ISDIR(linkStatBuf.st_mode)))
                            fileAttr |= FA_DIRECTORY;
                    }
                    fileAttr |= FA_SYS_FILE;
                }

                if (dirEntPtr->d_name[0] == '.' && dirEntPtr->d_name[1])
                    if (!(dirEntPtr->d_name[1] == '.' && !dirEntPtr->d_name[2]))
                        fileAttr |= FA_HIDDEN;

                if (euidaccess(fullName.c_str(), W_OK) != 0)
                    fileAttr |= FA_READ_ONLY;

                bool bIsSpecDir = (fileAttr & FA_DIRECTORY) && (name == "." || name == "..");

                if ((fileAttr & excludeAttr) == 0 && !bIsSpecDir)
                {
                    FileFindItem item;
                    item.fileSize = statBuf.st_size;
                    item.fileName = name;
                    item.attr = fileAttr;

                    findResult.push_back(item);
                }
            }
        } // while

        closedir(dirPtr);
    }
#endif
}

//-----------------------------------------------------------------------------
// 描述: 补全路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
string pathWithSlash(const string& path)
{
    string result = trimString(path);
    int len = (int)result.size();
    if (len > 0 && result[len-1] != PATH_DELIM)
        result += PATH_DELIM;
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 去掉路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
string pathWithoutSlash(const string& path)
{
    string result = trimString(path);
    int len = (int)result.size();
    if (len > 0 && result[len-1] == PATH_DELIM)
        result.resize(len - 1);
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件的全名(含绝对路径)
//-----------------------------------------------------------------------------
string GetAppExeName()
{
#ifdef ISE_WINDOWS
    char buffer[MAX_PATH];
    ::GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return string(buffer);
#endif
#ifdef ISE_LINUX
    const int BUFFER_SIZE = 1024;

    int r;
    char buffer[BUFFER_SIZE];
    string result;

    r = readlink("/proc/self/exe", buffer, BUFFER_SIZE);
    if (r != -1)
    {
        if (r >= BUFFER_SIZE) r = BUFFER_SIZE - 1;
        buffer[r] = 0;
        result = buffer;
    }
    else
    {
        iseThrowException(SEM_NO_PERM_READ_PROCSELFEXE);
    }

    return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
string getAppPath()
{
    return extractFilePath(GetAppExeName());
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径的子目录
//-----------------------------------------------------------------------------
string getAppSubPath(const string& subDir)
{
    return pathWithSlash(getAppPath() + subDir);
}

//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码
//-----------------------------------------------------------------------------
int getLastSysError()
{
#ifdef ISE_WINDOWS
    return ::GetLastError();
#endif
#ifdef ISE_LINUX
    return errno;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得当前线程的ID
//-----------------------------------------------------------------------------
THREAD_ID getCurThreadId()
{
#ifdef ISE_WINDOWS
    static __declspec (thread) THREAD_ID t_tid = 0;
    if (t_tid == 0)
        t_tid = ::GetCurrentThreadId();
    return t_tid;
#endif
#ifdef ISE_LINUX
    static __thread THREAD_ID t_tid = 0;
    if (t_tid == 0)
        t_tid = syscall(SYS_gettid);
    return t_tid;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码对应的错误信息
//-----------------------------------------------------------------------------
string sysErrorMessage(int errorCode)
{
#ifdef ISE_WINDOWS
    char *errorMsg;

    errorMsg = strerror(errorCode);
    return errorMsg;
#endif
#ifdef ISE_LINUX
    const int ERROR_MSG_SIZE = 256;
    char errorMsg[ERROR_MSG_SIZE];
    string result;

    errorMsg[0] = 0;
    strerror_r(errorCode, errorMsg, ERROR_MSG_SIZE);
    if (errorMsg[0] == 0)
        result = formatString("System error: %d", errorCode);
    else
        result = errorMsg;

    return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 睡眠 seconds 秒，可精确到纳秒。
// 参数:
//   seconds       - 睡眠的秒数，可为小数，可精确到纳秒 (实际精确度取决于操作系统)
//   allowInterrupt - 是否允许信号中断
//-----------------------------------------------------------------------------
void sleepSec(double seconds, bool allowInterrupt)
{
#ifdef ISE_WINDOWS
    ::Sleep((UINT)(seconds * 1000));
#endif
#ifdef ISE_LINUX
    const UINT NANO_PER_SEC = 1000000000;  // 一秒等于多少纳秒
    struct timespec req, remain;
    int r;

    req.tv_sec = (UINT)seconds;
    req.tv_nsec = (UINT)((seconds - req.tv_sec) * NANO_PER_SEC);

    while (true)
    {
        r = nanosleep(&req, &remain);
        if (r == -1 && errno == EINTR && !allowInterrupt)
            req = remain;
        else
            break;
    }
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得当前 Ticks，单位:毫秒
//-----------------------------------------------------------------------------
UINT getCurTicks()
{
#ifdef ISE_WINDOWS
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
UINT getTickDiff(UINT oldTicks, UINT newTicks)
{
    if (newTicks >= oldTicks)
        return (newTicks - oldTicks);
    else
        return (UINT(-1) - oldTicks + newTicks);
}

//-----------------------------------------------------------------------------
// 描述: 随机化 "随机数种子"
//-----------------------------------------------------------------------------
void randomize()
{
    srand((unsigned int)time(NULL));
}

//-----------------------------------------------------------------------------
// 描述: 返回 [min..max] 之间的一个随机数，包含边界
//-----------------------------------------------------------------------------
int getRandom(int min, int max)
{
    ISE_ASSERT((max - min) < MAXLONG);
    return min + (int)(((double)rand() / ((double)RAND_MAX + 0.1)) * (max - min + 1));
}

//-----------------------------------------------------------------------------
// 描述: 生成一组介于区间 [startNumber, endNumber] 内的不重复的随机数
// 注意: 数组 randomList 的容量必须 >= (endNumber - startNumber + 1)
//-----------------------------------------------------------------------------
void generateRandomList(int startNumber, int endNumber, int *randomList)
{
    if (startNumber > endNumber || !randomList) return;

    int count = endNumber - startNumber + 1;

    if (rand() % 2 == 0)
        for (int i = 0; i < count; i++)
            randomList[i] = startNumber + i;
    else
        for (int i = 0; i < count; i++)
            randomList[count - i - 1] = startNumber + i;

    for (int i = count - 1; i >= 1; i--)
        ise::swap(randomList[i], randomList[rand()%i]);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
