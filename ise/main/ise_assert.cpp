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
// 文件名称: ise_assert.cpp
// 功能描述: 断言支持
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_assert.h"
#include "ise/main/ise_exceptions.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// 描述: 内部使用的断言函数。输出日志，并抛出异常。
//-----------------------------------------------------------------------------
void internalAssert(const char *condition, const char *fileName, int lineNumber)
{
    SimpleException e(formatString("Assertion failed: %s", condition).c_str(), fileName, lineNumber);
    logger().writeException(e);
    throw e;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
