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
// ise_timer.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_TIMER_H_
#define _ISE_TIMER_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ÌáÇ°ÉùÃ÷

class EventLoop;
class EventLoopList;

///////////////////////////////////////////////////////////////////////////////
// class Timer

class Timer : boost::noncopyable
{
public:
    Timer(Timestamp expiration, INT64 interval, const TimerCallback& callback);

    Timestamp expiration() const { return expiration_; }
    INT64 interval() const { return interval_; }
    bool repeat() const { return repeat_; }
    TimerId timerId() const { return timerId_; }
    void invokeCallback();

    void restart(Timestamp now);

private:
    Timestamp expiration_;
    const INT64 interval_;  // millisecond
    const bool repeat_;
    TimerId timerId_;
    const TimerCallback callback_;

    static SeqNumberAlloc s_timerIdAlloc;
};

///////////////////////////////////////////////////////////////////////////////
// class TimerQueue

class TimerQueue : boost::noncopyable
{
public:
    TimerQueue();
    ~TimerQueue();

    void addTimer(Timer *timer);
    void cancelTimer(TimerId timerId);
    bool getNearestExpiration(Timestamp& expiration);
    void processExpiredTimers(Timestamp now);

private:
    void clearTimers();

private:
    typedef std::pair<Timestamp, Timer*> TimerItem;
    typedef std::set<TimerItem> TimerList;
    typedef std::map<TimerId, Timer*> TimerIdMap;
    typedef std::set<TimerId> TimerIds;

    TimerList timerList_;
    TimerIdMap timerIdMap_;
    bool callingExpiredTimers_;
    TimerIds cancelingTimers_;
};

///////////////////////////////////////////////////////////////////////////////
// class TimerManager

class TimerManager : boost::noncopyable
{
public:
    TimerManager();
    ~TimerManager();
public:
    TimerId executeAt(Timestamp time, const TimerCallback& callback);
    TimerId executeAfter(INT64 delay, const TimerCallback& callback);
    TimerId executeEvery(INT64 interval, const TimerCallback& callback);
    void cancelTimer(TimerId timerId);
private:
    EventLoop& getTimerEventLoop();
private:
    EventLoopList *eventLoopList_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_TIMER_H_
