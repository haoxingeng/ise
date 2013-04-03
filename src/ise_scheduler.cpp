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
// class CIseScheduleTask

CIseScheduleTask::CIseScheduleTask(UINT nTaskId, ISE_SCHEDULE_TASK_TYPE nTaskType,
	UINT nAfterSeconds, const SCH_TASK_TRIGGRE_CALLBACK& OnTrigger,
	const CCustomParams& CustomParams) :
		m_nTaskId(nTaskId),
		m_nTaskType(nTaskType),
		m_nAfterSeconds(nAfterSeconds),
		m_OnTrigger(OnTrigger),
		m_CustomParams(CustomParams),
		m_nLastTriggerTime(0)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 尝试处理此任务 (每秒执行一次)
//-----------------------------------------------------------------------------
void CIseScheduleTask::Process()
{
	int nCurYear, nCurMonth, nCurDay, nCurHour, nCurMinute, nCurSecond, nCurWeekDay, nCurYearDay;
	CDateTime::CurrentDateTime().DecodeDateTime(&nCurYear, &nCurMonth, &nCurDay,
		&nCurHour, &nCurMinute, &nCurSecond, &nCurWeekDay, &nCurYearDay);

	int nLastYear = -1, nLastMonth = -1, nLastDay = -1, nLastHour = -1, nLastWeekDay = -1;
	if (m_nLastTriggerTime != 0)
	{
		CDateTime(m_nLastTriggerTime).DecodeDateTime(&nLastYear, &nLastMonth, &nLastDay,
			&nLastHour, NULL, NULL, &nLastWeekDay);
	}

	UINT nElapsedSecs = (UINT)(-1);

	switch (m_nTaskType)
	{
	case STT_EVERY_HOUR:
		if (nCurHour != nLastHour)
		{
			nElapsedSecs = nCurMinute * SECONDS_PER_MINUTE + nCurSecond;
		}
		break;

	case STT_EVERY_DAY:
		if (nCurDay != nLastDay)
		{
			nElapsedSecs = nCurHour * SECONDS_PER_HOUR + nCurMinute * SECONDS_PER_MINUTE + nCurSecond;
		}
		break;

	case STT_EVERY_WEEK:
		if (nCurWeekDay != nLastWeekDay)
		{
			nElapsedSecs = nCurWeekDay * SECONDS_PER_DAY + nCurHour * SECONDS_PER_HOUR +
				nCurMinute * SECONDS_PER_MINUTE + nCurSecond;
		}
		break;

	case STT_EVERY_MONTH:
		if (nCurMonth != nLastMonth)
		{
			nElapsedSecs = nCurDay * SECONDS_PER_DAY + nCurHour * SECONDS_PER_HOUR +
				nCurMinute * SECONDS_PER_MINUTE + nCurSecond;
		}
		break;

	case STT_EVERY_YEAR:
		if (nCurYear != nLastYear)
		{
			nElapsedSecs = nCurYearDay * SECONDS_PER_DAY + nCurHour * SECONDS_PER_HOUR +
				nCurMinute * SECONDS_PER_MINUTE + nCurSecond;
		}
		break;
	}

	bool bTrigger = false;
	if (nElapsedSecs != (UINT)(-1))
	{
		if (m_nLastTriggerTime == 0)
		{
			// 如果之前从未触发过，若当前时间已越过时间点，则只可以在时间点附近触发
			const int MAX_SPAN_SECS = 10;
			bTrigger = (nElapsedSecs >= m_nAfterSeconds && nElapsedSecs <= m_nAfterSeconds + MAX_SPAN_SECS);
		}
		else
			bTrigger = (nElapsedSecs >= m_nAfterSeconds);
	}

	if (bTrigger)
	{
		m_nLastTriggerTime = time(NULL);

		if (m_OnTrigger.pProc)
			m_OnTrigger.pProc(m_OnTrigger.pParam, m_nTaskId, m_CustomParams);
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CIseScheduleTaskMgr

CIseScheduleTaskMgr::CIseScheduleTaskMgr() :
	m_TaskList(false, true),
	m_TaskIdAlloc(1)
{
	// nothing
}

CIseScheduleTaskMgr::~CIseScheduleTaskMgr()
{
	// nothing
}

//-----------------------------------------------------------------------------

CIseScheduleTaskMgr& CIseScheduleTaskMgr::Instance()
{
	static CIseScheduleTaskMgr obj;
	return obj;
}

//-----------------------------------------------------------------------------
// 描述: 由系统定时任务线程执行
//-----------------------------------------------------------------------------
void CIseScheduleTaskMgr::Execute(CThread& ExecutorThread)
{
	while (!ExecutorThread.GetTerminated())
	{
		try
		{
			CAutoLocker Locker(m_Lock);
			for (int i = 0; i < m_TaskList.GetCount(); i++)
				m_TaskList[i]->Process();
		}
		catch (...)
		{}

		SleepSec(1);
	}
}

//-----------------------------------------------------------------------------
// 描述: 添加一个任务
//-----------------------------------------------------------------------------
UINT CIseScheduleTaskMgr::AddTask(ISE_SCHEDULE_TASK_TYPE nTaskType, UINT nAfterSeconds,
	const SCH_TASK_TRIGGRE_CALLBACK& OnTrigger, const CCustomParams& CustomParams)
{
	CAutoLocker Locker(m_Lock);

	UINT nResult = m_TaskIdAlloc.AllocId();
	CIseScheduleTask *pTask = new CIseScheduleTask(nResult, nTaskType,
		nAfterSeconds, OnTrigger, CustomParams);
	m_TaskList.Add(pTask);

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 删除一个任务
//-----------------------------------------------------------------------------
bool CIseScheduleTaskMgr::RemoveTask(UINT nTaskId)
{
	CAutoLocker Locker(m_Lock);
	bool bResult = false; 

	for (int i = 0; i < m_TaskList.GetCount(); i++)
	{
		if (m_TaskList[i]->GetTaskId() == nTaskId)
		{
			m_TaskList.Delete(i);
			bResult = true;
			break;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 清空全部任务
//-----------------------------------------------------------------------------
void CIseScheduleTaskMgr::Clear()
{
	CAutoLocker Locker(m_Lock);
	m_TaskList.Clear();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
