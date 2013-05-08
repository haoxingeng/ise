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
// 文件名称: ise_err_msgs.h
// 功能描述: ISE错误信息定义
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_ERR_MSGS_H_
#define _ISE_ERR_MSGS_H_

#include "ise/main/ise_options.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 错误信息 (ISE Error Message)

// common
const char* const SEM_OUT_OF_MEMORY               = "Out of memory.";
const char* const SEM_FEATURE_NOT_SUPPORTED       = "This feature is not supported.";
const char* const SEM_INDEX_ERROR                 = "Index out of range.";
const char* const SEM_NO_PERM_READ_PROCSELFEXE    = "No permission to read /proc/self/exe.";

// ise_classes
const char* const SEM_INVALID_DATETIME_STR        = "Invalid datetime string.";
const char* const SEM_SEM_INIT_ERROR              = "Semaphore object init error.";
const char* const SEM_COND_INIT_ERROR             = "Condition object init error.";
const char* const SEM_FAIL_TO_CREATE_MUTEX        = "Fail to create mutex object.";
const char* const SEM_CANNOT_CREATE_FILE          = "Cannot create file '%s'. %s.";
const char* const SEM_CANNOT_OPEN_FILE            = "Cannot open file '%s'. %s.";
const char* const SEM_SET_FILE_STREAM_SIZE_ERR    = "Error setting file stream size.";
const char* const SEM_LIST_CAPACITY_ERROR         = "List capacity error.";
const char* const SEM_LIST_COUNT_ERROR            = "List count error.";
const char* const SEM_LIST_INDEX_ERROR            = "List index out of bounds (%d).";
const char* const SEM_SORTED_LIST_ERROR           = "Operation not allowed on sorted list.";
const char* const SEM_PROPLIST_NAME_ERROR         = "Invalid name in property list.";
const char* const SEM_STRINGS_NAME_ERROR          = "Invalid name in strings.";
const char* const SEM_DUPLICATE_STRING            = "String list does not allow duplicates.";
const char* const SEM_STREAM_READ_ERROR           = "Stream read error.";
const char* const SEM_STREAM_WRITE_ERROR          = "Stream write error.";
const char* const SEM_PACKET_UNPACK_ERROR         = "Packet unpack error.";
const char* const SEM_PACKET_PACK_ERROR           = "Packet pack error.";
const char* const SEM_UNSAFE_VALUE_IN_PACKET      = "Unsafe value in packet.";

// ise_thread
const char* const SEM_THREAD_RUN_ONCE             = "Thread::run() can be call only once.";

// ise_xml_doc
const char* const SEM_INVALID_XML_FILE_FORMAT     = "Invalid file format.";

// ise_application
const char* const SEM_BUSINESS_OBJ_EXPECTED       = "The IseBusiness object is null.";
const char* const SEM_SIG_TERM                    = "Exit Program. (signal: %d)";
const char* const SEM_SIG_FATAL_ERROR             = "Fatal Error. (signal: %d)";
const char* const SEM_ALREADY_RUNNING             = "The application is already running.";
const char* const SEM_INIT_DAEMON_ERROR           = "Init daemon error.";

// ise_server_*
const char* const SEM_CREATE_PIPE_ERROR           = "Fail to create pipe.";
const char* const SEM_CREATE_EPOLL_ERROR          = "Fail to create epoll object.";
const char* const SEM_EPOLL_WAIT_ERROR            = "epoll_wait error.";
const char* const SEM_EPOLL_CTRL_ERROR            = "epoll_ctl error (op: %d).";
const char* const SEM_THREAD_KILLED               = "Killed %d %s thread.";
const char* const SEM_IOCP_ERROR                  = "IOCP Error #%d";
const char* const SEM_INVALID_OP_FOR_IOCP         = "Invalid operation for IOCP.";
const char* const SEM_EVENT_LOOP_NOT_SPECIFIED    = "Event loop not specified.";

// ise_database
const char* const SEM_GET_CONN_FROM_POOL_ERROR    = "Cannot get connection from connection pool.";
const char* const SEM_FIELD_NAME_ERROR            = "Field name error: '%s'. Field list: [%s].";

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_ERR_MSGS_H_
