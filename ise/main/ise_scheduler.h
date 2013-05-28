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
// ise_scheduler.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SCHEDULER_H_
#define _ISE_SCHEDULER_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_global_defs.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class ScheduleTask;
class ScheduleTaskMgr;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

enum SCHEDULE_TASK_TYPE
{
    STT_EVERY_HOUR,
    STT_EVERY_DAY,
    STT_EVERY_WEEK,
    STT_EVERY_MONTH,
    STT_EVERY_YEAR,
};

typedef boost::function<void (UINT taskId, const Context& context)> SchTaskTriggerCallback;

///////////////////////////////////////////////////////////////////////////////
// 常量定义

const int SECONDS_PER_MINUTE = 60;
const int SECONDS_PER_HOUR   = 60*60;
const int SECONDS_PER_DAY    = 60*60*24;

///////////////////////////////////////////////////////////////////////////////
// class ScheduleTask

class ScheduleTask : boost::noncopyable
{
public:
    ScheduleTask(UINT taskId, SCHEDULE_TASK_TYPE taskType,
        UINT afterSeconds, const SchTaskTriggerCallback& onTrigger,
        const Context& context);
    ~ScheduleTask() {}

    void process();

    UINT getTaskId() const { return taskId_; }
    SCHEDULE_TASK_TYPE getTaskType() const { return taskType_; }
    UINT getAfterSeconds() const { return afterSeconds_; }

private:
    UINT taskId_;                         // 任务ID
    SCHEDULE_TASK_TYPE taskType_;     // 任务类型
    UINT afterSeconds_;                   // 按任务类型到达指定时间点后延后多少秒触发事件
    time_t lastTriggerTime_;              // 此任务上次触发事件的时间
    Context context_;                     // 自定义上下文
    SchTaskTriggerCallback onTrigger_;    // 触发事件回调
};

///////////////////////////////////////////////////////////////////////////////
// class ScheduleTaskMgr

class ScheduleTaskMgr : boost::noncopyable
{
public:
    ScheduleTaskMgr();
    ~ScheduleTaskMgr() {}

    void execute(Thread& executorThread);

    UINT addTask(SCHEDULE_TASK_TYPE taskType, UINT afterSeconds,
        const SchTaskTriggerCallback& onTrigger,
        const Context& context = EMPTY_CONTEXT);
    bool removeTask(UINT taskId);
    void clear();

private:
    typedef ObjectList<ScheduleTask> IseScheduleTaskList;

    IseScheduleTaskList taskList_;
    SeqNumberAlloc taskIdAlloc_;
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SCHEDULER_H_
