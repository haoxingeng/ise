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
// 文件名称: ise_scheduler.cpp
// 功能描述: 定时任务支持
///////////////////////////////////////////////////////////////////////////////

#include "ise_scheduler.h"
#include "ise_sysutils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class IseScheduleTask

IseScheduleTask::IseScheduleTask(UINT taskId, ISE_SCHEDULE_TASK_TYPE taskType,
	UINT afterSeconds, const SCH_TASK_TRIGGRE_CALLBACK& onTrigger,
	const CustomParams& customParams) :
		taskId_(taskId),
		taskType_(taskType),
		afterSeconds_(afterSeconds),
		onTrigger_(onTrigger),
		customParams_(customParams),
		lastTriggerTime_(0)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 尝试处理此任务 (每秒执行一次)
//-----------------------------------------------------------------------------
void IseScheduleTask::process()
{
	int curYear, curMonth, curDay, curHour, curMinute, curSecond, curWeekDay, curYearDay;
	DateTime::currentDateTime().decodeDateTime(&curYear, &curMonth, &curDay,
		&curHour, &curMinute, &curSecond, &curWeekDay, &curYearDay);

	int lastYear = -1, lastMonth = -1, lastDay = -1, lastHour = -1, lastWeekDay = -1;
	if (lastTriggerTime_ != 0)
	{
		DateTime(lastTriggerTime_).decodeDateTime(&lastYear, &lastMonth, &lastDay,
			&lastHour, NULL, NULL, &lastWeekDay);
	}

	UINT elapsedSecs = (UINT)(-1);

	switch (taskType_)
	{
	case STT_EVERY_HOUR:
		if (curHour != lastHour)
		{
			elapsedSecs = curMinute * SECONDS_PER_MINUTE + curSecond;
		}
		break;

	case STT_EVERY_DAY:
		if (curDay != lastDay)
		{
			elapsedSecs = curHour * SECONDS_PER_HOUR + curMinute * SECONDS_PER_MINUTE + curSecond;
		}
		break;

	case STT_EVERY_WEEK:
		if (curWeekDay != lastWeekDay)
		{
			elapsedSecs = curWeekDay * SECONDS_PER_DAY + curHour * SECONDS_PER_HOUR +
				curMinute * SECONDS_PER_MINUTE + curSecond;
		}
		break;

	case STT_EVERY_MONTH:
		if (curMonth != lastMonth)
		{
			elapsedSecs = curDay * SECONDS_PER_DAY + curHour * SECONDS_PER_HOUR +
				curMinute * SECONDS_PER_MINUTE + curSecond;
		}
		break;

	case STT_EVERY_YEAR:
		if (curYear != lastYear)
		{
			elapsedSecs = curYearDay * SECONDS_PER_DAY + curHour * SECONDS_PER_HOUR +
				curMinute * SECONDS_PER_MINUTE + curSecond;
		}
		break;
	}

	bool trigger = false;
	if (elapsedSecs != (UINT)(-1))
	{
		if (lastTriggerTime_ == 0)
		{
			// 如果之前从未触发过，若当前时间已越过时间点，则只可以在时间点附近触发
			const int MAX_SPAN_SECS = 10;
			trigger = (elapsedSecs >= afterSeconds_ && elapsedSecs <= afterSeconds_ + MAX_SPAN_SECS);
		}
		else
			trigger = (elapsedSecs >= afterSeconds_);
	}

	if (trigger)
	{
		lastTriggerTime_ = time(NULL);

		if (onTrigger_.proc)
			onTrigger_.proc(onTrigger_.param, taskId_, customParams_);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class IseScheduleTaskMgr

IseScheduleTaskMgr::IseScheduleTaskMgr() :
	taskList_(false, true),
	taskIdAlloc_(1)
{
	// nothing
}

IseScheduleTaskMgr::~IseScheduleTaskMgr()
{
	// nothing
}

//-----------------------------------------------------------------------------

IseScheduleTaskMgr& IseScheduleTaskMgr::instance()
{
	static IseScheduleTaskMgr obj;
	return obj;
}

//-----------------------------------------------------------------------------
// 描述: 由系统定时任务线程执行
//-----------------------------------------------------------------------------
void IseScheduleTaskMgr::execute(Thread& ExecutorThread)
{
	while (!ExecutorThread.isTerminated())
	{
		try
		{
			AutoLocker locker(lock_);
			for (int i = 0; i < taskList_.getCount(); i++)
				taskList_[i]->process();
		}
		catch (...)
		{}

		sleepSec(1);
	}
}

//-----------------------------------------------------------------------------
// 描述: 添加一个任务
//-----------------------------------------------------------------------------
UINT IseScheduleTaskMgr::addTask(ISE_SCHEDULE_TASK_TYPE taskType, UINT afterSeconds,
	const SCH_TASK_TRIGGRE_CALLBACK& onTrigger, const CustomParams& customParams)
{
	AutoLocker locker(lock_);

	UINT result = taskIdAlloc_.allocId();
	IseScheduleTask *task = new IseScheduleTask(result, taskType,
		afterSeconds, onTrigger, customParams);
	taskList_.add(task);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 删除一个任务
//-----------------------------------------------------------------------------
bool IseScheduleTaskMgr::removeTask(UINT taskId)
{
	AutoLocker locker(lock_);
	bool result = false; 

	for (int i = 0; i < taskList_.getCount(); i++)
	{
		if (taskList_[i]->getTaskId() == taskId)
		{
			taskList_.del(i);
			result = true;
			break;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 清空全部任务
//-----------------------------------------------------------------------------
void IseScheduleTaskMgr::clear()
{
	AutoLocker locker(lock_);
	taskList_.clear();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
