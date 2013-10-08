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
// 文件名称: ise_timer.cpp
// 功能描述: 定时器的实现
///////////////////////////////////////////////////////////////////////////////

#include "ise/main/ise_timer.h"
#include "ise/main/ise_err_msgs.h"

using namespace ise;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class Timer

SeqNumberAlloc Timer::s_timerIdAlloc(1);

Timer::Timer(Timestamp expiration, double interval, const TimerCallback& callback) :
    expiration_(expiration),
    interval_(interval),
    repeat_(interval > 0.0),
    timerId_(s_timerIdAlloc.allocId()),
    callback_(callback)
{
    // nothing
}

//-----------------------------------------------------------------------------

void Timer::invokeCallback()
{
    try
    {
        if (callback_)
            callback_();
    }
    catch (Exception& e)
    {
        logger().writeException(e);
    }
    catch (...)
    {}
}

//-----------------------------------------------------------------------------

void Timer::restart(Timestamp now)
{
    if (repeat_)
        expiration_ = now + interval_ * MICROSECS_PER_SECOND;
    else
        expiration_ = Timestamp(0);
}

///////////////////////////////////////////////////////////////////////////////
// class TimerQueue

TimerQueue::TimerQueue() :
    callingExpiredTimers_(false)
{
    // nothing
}

TimerQueue::~TimerQueue()
{
    clearTimers();
}

//-----------------------------------------------------------------------------

void TimerQueue::addTimer(Timer *timer)
{
    bool success = timerList_.insert(TimerItem(timer->expiration(), timer)).second;
    ISE_ASSERT(success);
    (void)success;

    success = timerIdMap_.insert(std::make_pair(timer->timerId(), timer)).second;
    ISE_ASSERT(success);

    ISE_ASSERT(timerList_.size() == timerIdMap_.size());
}

//-----------------------------------------------------------------------------

void TimerQueue::cancelTimer(TimerId timerId)
{
    TimerIdMap::iterator iter = timerIdMap_.find(timerId);
    if (iter != timerIdMap_.end())
    {
        Timer *timer = iter->second;
        timerList_.erase(TimerItem(timer->expiration(), timer));
        timerIdMap_.erase(iter);

        delete timer;

        ISE_ASSERT(timerList_.size() == timerIdMap_.size());
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timerId);
    }
}

//-----------------------------------------------------------------------------

bool TimerQueue::getNearestExpiration(Timestamp& expiration)
{
    bool result = !timerList_.empty();
    if (result)
        expiration = timerList_.begin()->first;
    return result;
}

//-----------------------------------------------------------------------------

void TimerQueue::processExpiredTimers(Timestamp now)
{
    std::vector<Timer*> expiredTimers;
    TimerItem timerItem(now, reinterpret_cast<Timer*>(std::numeric_limits<uintptr_t>::max()));

    TimerList::iterator bound = timerList_.upper_bound(timerItem);
    for (TimerList::iterator iter = timerList_.begin(); iter != bound;)
    {
        Timer *timer = iter->second;
        expiredTimers.push_back(timer);

        timerList_.erase(iter++);
        timerIdMap_.erase(timer->timerId());

        ISE_ASSERT(timerList_.size() == timerIdMap_.size());
    }

    for (size_t i = 0; i < expiredTimers.size(); i++)
    {
        Timer *timer = expiredTimers[i];

        cancelingTimers_.clear();
        callingExpiredTimers_ = true;
        timer->invokeCallback();
        callingExpiredTimers_ = false;

        if (timer->repeat() &&
            cancelingTimers_.find(timer->timerId()) == cancelingTimers_.end())
        {
            timer->restart(now);
            addTimer(timer);
        }
        else
            delete timer;
    }
}

//-----------------------------------------------------------------------------

void TimerQueue::clearTimers()
{
    for (TimerList::iterator iter = timerList_.begin(); iter != timerList_.end(); ++iter)
        delete iter->second;
    timerList_.clear();
    timerIdMap_.clear();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
