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
// 文件名称: ise_epoll.cpp
// 功能描述: EPoll 实现
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_epoll.h"
#include "ise/main/ise_err_msgs.h"
#include "ise/main/ise_event_loop.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////

#ifdef ISE_LINUX

///////////////////////////////////////////////////////////////////////////////
// class EpollObject

EpollObject::EpollObject(EventLoop *eventLoop) :
    eventLoop_(eventLoop)
{
    events_.resize(INITIAL_EVENT_SIZE);
    createEpoll();
    createPipe();
}

EpollObject::~EpollObject()
{
    destroyPipe();
    destroyEpoll();
}

//-----------------------------------------------------------------------------
// 描述: 执行一次 EPoll 轮循
//-----------------------------------------------------------------------------
void EpollObject::poll()
{
    int timeout = eventLoop_->calcLoopWaitTimeout();

    int eventCount = ::epoll_wait(epollFd_, &events_[0], (int)events_.size(), timeout);

    if (timeout != TIMEOUT_INFINITE)
        eventLoop_->processExpiredTimers();

    if (eventCount > 0)
    {
        processEvents(eventCount);

        if (eventCount == (int)events_.size())
            events_.resize(eventCount * 2);
    }
    else if (eventCount < 0)
    {
        logger().writeStr(SEM_EPOLL_WAIT_ERROR);
    }
}

//-----------------------------------------------------------------------------
// 描述: 唤醒正在阻塞的 Poll() 函数
//-----------------------------------------------------------------------------
void EpollObject::wakeup()
{
    BYTE val = 0;
    ::write(pipeFds_[1], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// 描述: 向 EPoll 中添加一个连接
//-----------------------------------------------------------------------------
void EpollObject::addConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_ADD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 更新 EPoll 中的一个连接
//-----------------------------------------------------------------------------
void EpollObject::updateConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_MOD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// 描述: 从 EPoll 中删除一个连接
//-----------------------------------------------------------------------------
void EpollObject::removeConnection(BaseTcpConnection *connection)
{
    epollControl(
        EPOLL_CTL_DEL, connection, connection->getSocket().getHandle(),
        false, false);
}

//-----------------------------------------------------------------------------
// 描述: 设置回调
//-----------------------------------------------------------------------------
void EpollObject::setNotifyEventCallback(const NotifyEventCallback& callback)
{
    onNotifyEvent_ = callback;
}

//-----------------------------------------------------------------------------

void EpollObject::createEpoll()
{
    epollFd_ = ::epoll_create(1024);
    if (epollFd_ < 0)
        logger().writeStr(SEM_CREATE_EPOLL_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyEpoll()
{
    if (epollFd_ > 0)
        ::close(epollFd_);
}

//-----------------------------------------------------------------------------

void EpollObject::createPipe()
{
    // pipeFds_[0] for reading, pipeFds_[1] for writing.
    memset(pipeFds_, 0, sizeof(pipeFds_));
    if (::pipe(pipeFds_) == 0)
        epollControl(EPOLL_CTL_ADD, NULL, pipeFds_[0], false, true);
    else
        logger().writeStr(SEM_CREATE_PIPE_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyPipe()
{
    epollControl(EPOLL_CTL_DEL, NULL, pipeFds_[0], false, false);

    if (pipeFds_[0]) close(pipeFds_[0]);
    if (pipeFds_[1]) close(pipeFds_[1]);

    memset(pipeFds_, 0, sizeof(pipeFds_));
}

//-----------------------------------------------------------------------------

void EpollObject::epollControl(int operation, void *param, int handle,
    bool enableSend, bool enableRecv)
{
    // 注: 采用 Level Triggered (LT, 也称 "电平触发") 模式

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = param;
    if (enableSend)
        event.events |= EPOLLOUT;
    if (enableRecv)
        event.events |= (EPOLLIN | EPOLLPRI);

    if (::epoll_ctl(epollFd_, operation, handle, &event) < 0)
    {
        logger().writeFmt(SEM_EPOLL_CTRL_ERROR, operation);
    }
}

//-----------------------------------------------------------------------------
// 描述: 处理管道事件
//-----------------------------------------------------------------------------
void EpollObject::processPipeEvent()
{
    BYTE val;
    ::read(pipeFds_[0], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// 描述: 处理 EPoll 轮循后的事件
//-----------------------------------------------------------------------------
void EpollObject::processEvents(int eventCount)
{
/*
// from epoll.h
enum EPOLL_EVENTS
{
    EPOLLIN      = 0x001,
    EPOLLPRI     = 0x002,
    EPOLLOUT     = 0x004,
    EPOLLRDNORM  = 0x040,
    EPOLLRDBAND  = 0x080,
    EPOLLWRNORM  = 0x100,
    EPOLLWRBAND  = 0x200,
    EPOLLMSG     = 0x400,
    EPOLLERR     = 0x008,
    EPOLLHUP     = 0x010,
    EPOLLRDHUP   = 0x2000,
    EPOLLONESHOT = (1 << 30),
    EPOLLET      = (1 << 31)
};
*/

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

    for (int i = 0; i < eventCount; i++)
    {
        epoll_event& ev = events_[i];
        if (ev.data.ptr == NULL)  // for pipe
        {
            processPipeEvent();
        }
        else
        {
            BaseTcpConnection *connection = (BaseTcpConnection*)ev.data.ptr;
            EVENT_TYPE eventType = ET_NONE;

            //logger().writeFmt("processEvents: %u", ev.events);  // debug

            if ((ev.events & EPOLLERR) || ((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN)))
                eventType = ET_ERROR;
            else if (ev.events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
                eventType = ET_ALLOW_RECV;
            else if (ev.events & EPOLLOUT)
                eventType = ET_ALLOW_SEND;

            if (eventType != ET_NONE && onNotifyEvent_)
                onNotifyEvent_(connection, eventType);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef ISE_LINUX */

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
