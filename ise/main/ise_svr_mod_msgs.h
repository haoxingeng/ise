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
// ise_svr_mod_msgs.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SVR_MOD_MSGS_H_
#define _ISE_SVR_MOD_MSGS_H_

#include "ise.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 消息代码定义 (Server Module Message Code)

const int SMMC_BASE = 100;

///////////////////////////////////////////////////////////////////////////////
// 消息结构定义

// 消息基类
struct BaseSvrModMessage
{
public:
    UINT messageCode;      // 消息代码
    bool isHandled;        // 是否处理了此消息
public:
    BaseSvrModMessage() : messageCode(0), isHandled(false) {}
    virtual ~BaseSvrModMessage() {}
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SVR_MOD_MSGS_H_
