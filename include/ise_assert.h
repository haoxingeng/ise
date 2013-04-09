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
// ise_assert.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_ASSERT_H_
#define _ISE_ASSERT_H_

namespace ise
{

///////////////////////////////////////////////////////////////////////////////

extern void internalAssert(const char *condition, const char *fileName, int lineNumber);

#ifdef ISE_DEBUG
#define ISE_ASSERT(c)   ((c) ? (void)0 : internalAssert(#c, __FILE__, __LINE__))
#else
#define ISE_ASSERT(c)   ((void)0)
#endif

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_ASSERT_H_
