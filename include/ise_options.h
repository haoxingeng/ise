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
// 文件名称: ise_options.h
// 功能描述: 全局配置
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_OPTIONS_H_
#define _ISE_OPTIONS_H_

///////////////////////////////////////////////////////////////////////////////
// 平台定义

#if defined(__BORLANDC__)
#define ISE_WINDOWS
#define ISE_COMPILER_BCB
#endif

#if defined(_MSC_VER)
#define ISE_WINDOWS
#define ISE_COMPILER_VC
#endif

#if defined(__GNUC__)
#define ISE_LINUX
#define ISE_COMPILER_GCC
#endif

///////////////////////////////////////////////////////////////////////////////
// 调试开关

#define ISE_DEBUG

#include "ise_assert.h"

///////////////////////////////////////////////////////////////////////////////
// 编译器相关设置

// 关闭警告: "cannot create pre-compiled header"
#ifdef ISE_COMPILER_BCB
#pragma warn -pch
#endif

#ifdef ISE_COMPILER_VC
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#pragma warning(disable: 4355)
#pragma warning(disable: 4244)
#pragma warning(disable: 4311)
#pragma warning(disable: 4312)
#endif

///////////////////////////////////////////////////////////////////////////////
// 其它

// 防止 winsock2.h 和 winsock.h 同时包含引起冲突
#ifdef ISE_WINDOWS
#define _WINSOCKAPI_
#endif

// 是否使用 "非标准STL"
#define ISE_USING_EXT_STL

///////////////////////////////////////////////////////////////////////////////

#endif // _ISE_OPTIONS_H_
