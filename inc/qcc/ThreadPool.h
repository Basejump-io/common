/**
 * @file
 *
 * Simple ThreadPool using Timers and Alarms
 */

/******************************************************************************
 * Copyright 2012, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/
#ifndef _QCC_THREADPOOL_H
#define _QCC_THREADPOOL_H

#include <qcc/Timer.h>

namespace qcc {

class Runnable : private qcc::AlarmListener {
public:
    virtual void Run(void) = 0;

private:
    friend class ThreadPool;
    virtual void AlarmTriggered(const Alarm& alarm, QStatus reason)
    {
        Run();
    }
};

class ThreadPool {
public:
    ThreadPool(const char* name, uint32_t poolSize) : dispatcher(name, false, poolSize)
    {
        dispatcher.Start();
    }

    virtual ~ThreadPool()
    {
        dispatcher.Stop();
        dispatcher.Join();
    }

    void Execute(Runnable& runnable) 
    {
        qcc::Alarm alarm = qcc::Alarm(0, &runnable, 0, NULL);
        dispatcher.AddAlarm(alarm);
    }

private:
    qcc::Timer dispatcher;
};

} // namespace qcc

#endif
