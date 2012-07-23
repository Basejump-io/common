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

namespace qcc {

class Timer;
class _Alarm;

typedef qcc::ManagedObj<_Alarm> Alarm;

class OSAlarm {
  protected:
    OSAlarm();

    void UpdateComputedTime(Timespec absoluteTime);

    Windows::System::Threading::ThreadPoolTimer ^ _timer;
    qcc::CountDownLatch _latch;
    uint64_t computedTimeMillis;
};

class OSTimer {
  public:
    void AllocThreadState();
    void DeleteThreadState();
    void MarshalThreadState(void* srcThread, void* destThread);

  protected:
    OSTimer(qcc::Timer* timer);

    void TimerCallback(Windows::System::Threading::ThreadPoolTimer ^ timer);

    qcc::Timer* _timer;
    std::map<void*, qcc::Alarm> _timerMap;
    std::map<void*, bool> _timerHasOwnership;
};

}

#endif //_QCC_OSTIMER_H
