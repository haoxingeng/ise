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
// ise_epoll.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EPOLL_H_
#define _ISE_EPOLL_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_socket.h"
#include "ise/main/ise_exceptions.h"

#ifdef ISE_LINUX
#include <sys/epoll.h>
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// classes

#ifdef ISE_LINUX
class EpollObject;
#endif

// 提前声明
class EventLoop;

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class EpollObject - Linux EPoll 功能封装

class EpollObject
{
public:
    enum { INITIAL_EVENT_SIZE = 32 };

    enum EVENT_TYPE
    {
        ET_NONE        = 0,
        ET_ERROR       = 1,
        ET_ALLOW_SEND  = 2,
        ET_ALLOW_RECV  = 3,
    };

    typedef std::vector<struct epoll_event> EventList;
    typedef int EventPipe[2];

    typedef boost::function<void (BaseTcpConnection *connection, EVENT_TYPE eventType)> NotifyEventCallback;

public:
    EpollObject(EventLoop *eventLoop);
    ~EpollObject();

    void poll();
    void wakeup();

    void addConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv);
    void updateConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv);
    void removeConnection(BaseTcpConnection *connection);

    void setNotifyEventCallback(const NotifyEventCallback& callback);

private:
    void createEpoll();
    void destroyEpoll();
    void createPipe();
    void destroyPipe();

    void epollControl(int operation, void *param, int handle, bool enableSend, bool enableRecv);

    void processPipeEvent();
    void processEvents(int eventCount);

private:
    EventLoop *eventLoop_;        // 所属 EventLoop
    int epollFd_;                 // EPoll 的文件描述符
    EventList events_;            // 存放 epoll_wait() 返回的事件
    EventPipe pipeFds_;           // 用于唤醒 epoll_wait() 的管道
    NotifyEventCallback onNotifyEvent_;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_EPOLL_H_
