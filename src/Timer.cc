/**
 * @file
 *
 * Timer thread
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

#include <qcc/platform.h>

#include <qcc/Debug.h>
#include <qcc/Timer.h>

#define QCC_MODULE  "TIMER"

#define FALLBEHIND_WARNING_MS 500

using namespace std;
using namespace qcc;

void Timer::AddAlarm(const Alarm& alarm)
{
    lock.Lock();
    bool alertThread = alarms.empty() || (alarm < *alarms.begin());
    alarms.insert(alarm);
    lock.Unlock();

    if (alertThread && !inUserCallback) {
        Alert();
    }
}

void Timer::RemoveAlarm(const Alarm& alarm)
{
    lock.Lock();
    alarms.erase(alarm);
    lock.Unlock();
}

ThreadReturn STDCALL Timer::Run(void* arg)
{
    /* Wait for first entry on (sorted) alarm list to expire */
    while (!IsStopping()) {
        Timespec now;
        GetTimeNow(&now);
        lock.Lock();
        if (!alarms.empty()) {
            const Alarm& topAlarm = *alarms.begin();
            int64_t delay = topAlarm.alarmTime - now;
            if (0 < delay) {
                lock.Unlock();
                Event evt(static_cast<uint32_t>(delay), 0);
                Event::Wait(evt);
                stopEvent.ResetEvent();

            } else {
                set<Alarm>::iterator it = alarms.begin();
                Alarm top = *it;
                alarms.erase(it);
                inUserCallback = true;
                lock.Unlock();
                if (FALLBEHIND_WARNING_MS < -delay) {
                    QCC_LogError(ER_TIMER_FALLBEHIND, ("Timer has fallen behind by %d ms", -delay));
                }
                (top.listener->AlarmTriggered)(top);
                inUserCallback = false;
                if (0 != top.periodMs) {
                    top.alarmTime += top.periodMs;
                    if (top.alarmTime < now) {
                        top.alarmTime = now;
                    }
                    AddAlarm(top);
                }
            }
        } else {
            lock.Unlock();
            Event evt(Event::WAIT_FOREVER, 0);
            Event::Wait(evt);
            stopEvent.ResetEvent();
        }
    }
    return (ThreadReturn) 0;
}



