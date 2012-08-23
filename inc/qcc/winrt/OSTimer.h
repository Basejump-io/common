/**
 * @file
 *
 * OSTimer
 */

/******************************************************************************
 * Copyright 2009-2011, Qualcomm Innovation Center, Inc.
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
#ifndef _QCC_OSTIMER_H
#define _QCC_OSTIMER_H

#include <qcc/CountDownLatch.h>
#include <ctxtcall.h>
#include <ppltasks.h>

namespace qcc {

class Timer;
class _Alarm;

typedef qcc::ManagedObj<_Alarm> Alarm;

class OSAlarm {
  protected:
    friend class OSTimer;
    OSAlarm();

    void UpdateComputedTime(Timespec absoluteTime);

    Windows::System::Threading::ThreadPoolTimer ^ _timer;
    qcc::CountDownLatch _latch;
    uint64_t computedTimeMillis;
};

class OSTimer {
  protected:
    OSTimer(qcc::Timer* timer);
    ~OSTimer();

    void TimerCallback(Windows::System::Threading::ThreadPoolTimer ^ timer);
    void TimerCleanupCallback(Windows::System::Threading::ThreadPoolTimer ^ timer);
    void StopInternal(bool timerExiting);

    qcc::Timer* _timer;
    std::map<void*, qcc::Alarm> _timerMap;
    qcc::_CountDownLatch _timersCountdownLatch;
    std::map<void*, bool> _timerHasOwnership;
    concurrency::task<void>* _stopTask;
};

}

#endif //_QCC_OSTIMER_H
