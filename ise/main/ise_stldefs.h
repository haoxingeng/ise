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
// 文件名称: ise_stldefs.h
// 功能描述: STL移植性处理
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_STLDEFS_H_
#define _ISE_STLDEFS_H_

#include "ise/main/ise_options.h"

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <deque>
#include <algorithm>

using std::string;

///////////////////////////////////////////////////////////////////////////////
// "非标准STL" 包含文件

#ifdef ISE_USING_EXT_STL
  #ifdef ISE_COMPILER_BCB
    #if (__BORLANDC__ >= 0x590)   // BCB2007
      #include <dinkumware/hash_map>
      #include <dinkumware/hash_set>
    #else
      #include <stlport/hash_map>
      #include <stlport/hash_set>
    #endif
  #endif
  #ifdef ISE_COMPILER_VC
    #include <hash_map>
    #include <hash_set>
  #endif
  #ifdef ISE_COMPILER_GCC
    #include <ext/hash_map>
    #include <ext/hash_set>
  #endif
#endif

///////////////////////////////////////////////////////////////////////////////
// "非标准STL" 命名空间

#ifdef ISE_COMPILER_BCB
  #define EXT_STL_NAMESPACE   std
#endif

#ifdef ISE_COMPILER_VC
  #define EXT_STL_NAMESPACE   stdext
#endif

#ifdef ISE_COMPILER_GCC
  #define EXT_STL_NAMESPACE   __gnu_cxx
#endif

///////////////////////////////////////////////////////////////////////////////
// string hasher 定义

#ifdef ISE_USING_EXT_STL
#ifdef ISE_LINUX
namespace EXT_STL_NAMESPACE
{
    template <class _CharT, class _Traits, class _Alloc>
    inline size_t stl_string_hash(const basic_string<_CharT,_Traits,_Alloc>& s)
    {
        unsigned long h = 0;
        size_t len = s.size();
        const _CharT* data = s.data();
        for (size_t i = 0; i < len; ++i)
            h = 5 * h + data[i];
        return size_t(h);
    }

    template <> struct hash<string> {
        size_t operator()(const string& s) const { return stl_string_hash(s); }
    };

    template <> struct hash<wstring> {
        size_t operator()(const wstring& s) const { return stl_string_hash(s); }
    };
}
#endif
#endif

///////////////////////////////////////////////////////////////////////////////

#endif // _ISE_STLDEFS_H_
