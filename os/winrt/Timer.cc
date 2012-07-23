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
#include <qcc/CountDownLatch.h>
#include <Status.h>
#include <list>
#include <map>

#define QCC_MODULE  "TIMER"

#define WORKER_IDLE_TIMEOUT_MS  20
#define FALLBEHIND_WARNING_MS   500
#define HUNDRED_NANOSECONDS_PER_MILLISECOND 10000
#define MILLISECONDS_PER_SECOND 1000

using namespace std;
using namespace qcc;
using namespace Windows::System::Threading;

int32_t qcc::_Alarm::nextId = 0;

namespace qcc {

_Alarm::_Alarm() : listener(NULL), periodMs(0), context(NULL), id(IncrementAndFetch(&nextId))
{
}

_Alarm::_Alarm(Timespec absoluteTime, AlarmListener* listener, void* context, uint32_t periodMs)
    : alarmTime(absoluteTime), listener(listener), periodMs(periodMs), context(context), id(IncrementAndFetch(&nextId))
{
    UpdateComputedTime(alarmTime);
}

_Alarm::_Alarm(uint32_t relativeTime, AlarmListener* listener, void* context, uint32_t periodMs)
    : alarmTime(), listener(listener), periodMs(periodMs), context(context), id(IncrementAndFetch(&nextId))
{
    if (relativeTime == WAIT_FOREVER) {
        alarmTime = END_OF_TIME;
    } else {
        GetTimeNow(&alarmTime);
        alarmTime += relativeTime;
    }
    UpdateComputedTime(alarmTime);
}

_Alarm::_Alarm(AlarmListener* listener, void* context)
    : alarmTime(0, TIME_RELATIVE), listener(listener), periodMs(0), context(context), id(IncrementAndFetch(&nextId))
{
    UpdateComputedTime(alarmTime);
}

void* _Alarm::GetContext(void) const
{
    return context;
}

void _Alarm::SetContext(void* c) const
{
    context = c;
}

uint64_t _Alarm::GetAlarmTime() const
{
    return alarmTime.GetAbsoluteMillis();
}

bool _Alarm::operator<(const _Alarm& other) const
{
    return (id < other.id);
}

bool _Alarm::operator==(const _Alarm& other) const
{
    return (id == other.id);
}

void OSAlarm::UpdateComputedTime(Timespec absoluteTime)
{
    uint64_t now = GetTimestamp64();
    computedTimeMillis = absoluteTime.GetAbsoluteMillis() - now;
    if (computedTimeMillis > now) {
        computedTimeMillis = 0;
    }
}

Timer::Timer(const char* name, bool expireOnExit, uint32_t concurency, bool preventReentrancy)
    : expireOnExit(expireOnExit), timerThreads(concurency), isRunning(false), controllerIdx(0), OSTimer(this)
{
}

Timer::~Timer()
{
    lock.Lock();
    for (multiset<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        const Alarm& alarm = *it;
        if (nullptr != alarm->_timer) {
            try {
                alarm->_timer->Cancel();
            } catch (...) {
                // don't bubble OS exceptions out
            }
        }
        // Execution here is a sequential flush to notify listeners of exit
        alarm->listener->AlarmTriggered(alarm, ER_TIMER_EXITING);
    }
    alarms.clear();
    _timerMap.clear();
    lock.Unlock();
}

QStatus Timer::Start()
{
    QStatus status = ER_OK;
    lock.Lock();
    if (!isRunning) {
        for (multiset<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
            Alarm& a = (Alarm) * it;
            try {
                Windows::Foundation::TimeSpan ts = { a->computedTimeMillis * HUNDRED_NANOSECONDS_PER_MILLISECOND };
                ThreadPoolTimer ^ tpt = ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([&] (ThreadPoolTimer ^ timer) {
                                                                                                     OSTimer::TimerCallback(timer);
                                                                                                 }), ts);
                a->_timer = tpt;
                _timerMap[(void*)tpt] = a;
            } catch (...) {
                status = ER_FAIL;
                break;
            }
        }
        isRunning = (status == ER_OK);
    }
    lock.Unlock();
    return status;
}

void Timer::TimerCallback(void* context)
{
    lock.Lock();
    bool alarmFound = false;
    qcc::Alarm alarm;
    if (_timerMap.find(context) != _timerMap.end()) {
        alarmFound = true;
        alarm = _timerMap[context]; // ThreadPoolTimer -> alarm
        alarm->_latch->Increment();
        // Ensure a single alarm can only be once in the map
        if (alarm->periodMs == 0) {
            // Single shot timer exiting
            RemoveAlarm(alarm, false);
        } else {
            qcc::Alarm newAlarm(alarm);
            // Schedule based on the period. Don't bother chasing the nanoseconds.
            ReplaceAlarm(alarm, newAlarm, false);
        }
    }
    // Set the default state for the callback
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    _timerHasOwnership[timerThreadHandle] = true;
    lock.Unlock();
    reentrancyLock.Lock();
    if (alarmFound) {
        alarm->listener->AlarmTriggered(alarm, ER_OK);
        alarm->_latch->Decrement();
    }
    lock.Lock();
    if (_timerHasOwnership[timerThreadHandle]) {
        reentrancyLock.Unlock();
    }
    // Make sure we don't grow the map unbounded
    _timerHasOwnership.erase(timerThreadHandle);
    lock.Unlock();
}

QStatus Timer::Stop()
{
    QStatus status = ER_OK;
    lock.Lock();
    for (multiset<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        const Alarm& alarm = *it;
        if (nullptr != alarm->_timer) {
            try {
                alarm->_timer->Cancel();
            } catch (...) {
                // don't bubble OS exceptions out
            }
        }
    }
    lock.Unlock();
    return status;
}

QStatus Timer::Join()
{
    QStatus status = ER_OK;
    qcc::Event evt;
    while (true) {
        lock.Lock();
        size_t timerCount = alarms.size();
        lock.Unlock();
        if (0 == timerCount) {
            // Spin until the list is empty
            qcc::Event::Wait(evt, 5);
        }
    }
    return status;
}

QStatus Timer::AddAlarm(const Alarm& alarm)
{
    QStatus status = ER_OK;
    lock.Lock();
    if (isRunning) {
        try {
            Windows::Foundation::TimeSpan ts = { alarm->computedTimeMillis * HUNDRED_NANOSECONDS_PER_MILLISECOND };
            ThreadPoolTimer ^ tpt = ThreadPoolTimer::CreateTimer(ref new TimerElapsedHandler([&] (ThreadPoolTimer ^ timer) {
                                                                                                 OSTimer::TimerCallback(timer);
                                                                                             }), ts);
            Alarm& a = (Alarm)alarm;
            a->_timer = tpt;
            _timerMap[(void*)tpt] = a;
            alarms.insert(a);
        } catch (...) {
            status = ER_FAIL;
        }
    } else {
        alarms.insert(alarm);
    }
    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    bool removed = false;
    qcc::Event evt;
    lock.Lock();
    multiset<Alarm>::iterator it = alarms.find(alarm);
    if (it != alarms.end()) {
        Alarm a = (Alarm) * it;
        // Remove the lookaside
        std::map<void*, qcc::Alarm>::iterator itTimerMap = _timerMap.find((void*)a->_timer);
        if (itTimerMap != _timerMap.end()) {
            _timerMap.erase(itTimerMap);
        }
        // Cancel the firing (if possible)
        if (nullptr != a->_timer) {
            try {
                a->_timer->Cancel();
            } catch (...) {
                // Don't bubble OS exceptions out
            }
        }
        // Wait for triggering to finish if specified
        while (blockIfTriggered && a->_latch->Current() != 0) {
            lock.Unlock();
            a->_latch->Wait();
            lock.Lock();
        }
        // Clear out the alarm if someone didn't erase it before re-acquiring the lock
        it = alarms.find(a);
        if (it != alarms.end()) {
            alarms.erase(it);
            removed = true;
        }
    }
    lock.Unlock();
    return removed;
}

QStatus Timer::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    QStatus status = ER_NO_SUCH_ALARM;
    qcc::Event evt;
    lock.Lock();
    multiset<Alarm>::iterator it = alarms.find(origAlarm);
    if (it != alarms.end()) {
        Alarm a = (Alarm) * it;
        // Remove the lookaside
        std::map<void*, qcc::Alarm>::iterator itTimerMap = _timerMap.find((void*)a->_timer);
        if (itTimerMap != _timerMap.end()) {
            _timerMap.erase(itTimerMap);
        }
        // Cancel the firing (if possible)
        if (nullptr != a->_timer) {
            try {
                a->_timer->Cancel();
            } catch (...) {
                // Don't bubble OS exceptions out
            }
        }
        // Wait for triggering to finish if specified
        while (blockIfTriggered && a->_latch->Current() != 0) {
            lock.Unlock();
            a->_latch->Wait();
            lock.Lock();
        }
        // Clear out the alarm if someone didn't erase it before re-acquiring the lock
        it = alarms.find(a);
        if (it != alarms.end()) {
            alarms.erase(it);
        }
        // Ensure the ids are the same as this is replace
        Alarm& na = (Alarm)newAlarm;
        na->id = origAlarm->id;
        status = AddAlarm(na);
    }
    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const AlarmListener& listener, Alarm& alarm)
{
    bool foundOne = false;
    lock.Lock();
    for (multiset<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
        if ((*it)->listener == &listener) {
            const Alarm a = *it;
            lock.Unlock();
            RemoveAlarm(a, true);
            foundOne = true;
            lock.Lock();
            // iterator is invalid, re-initialize
            it = alarms.begin();
        }
    }
    lock.Unlock();
    return foundOne;
}

void Timer::RemoveAlarmsWithListener(const AlarmListener& listener)
{
    Alarm a;
    while (RemoveAlarm(listener, a)) {
    }
}

bool Timer::HasAlarm(const Alarm& alarm)
{
    bool ret = false;
    lock.Lock();
    if (isRunning) {
        ret = alarms.count(alarm) != 0;
    }
    lock.Unlock();
    return ret;
}

void Timer::ThreadExit(Thread* thread)
{
    // never called
}

void Timer::EnableReentrancy()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    lock.Lock();
    if (_timerHasOwnership.find(timerThreadHandle) != _timerHasOwnership.end()) {
        if (_timerHasOwnership[timerThreadHandle]) {
            reentrancyLock.Unlock();
            _timerHasOwnership[timerThreadHandle] = false;
        }
    } else {
        QCC_DbgPrintf(("Invalid call to Timer::EnableReentrancy from thread %s; only allowed from %s", Thread::GetThreadName(), nameStr.c_str()));
    }
    lock.Unlock();
}

bool Timer::ThreadHoldsLock()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    lock.Lock();
    bool retVal = false;
    if (_timerHasOwnership.find(timerThreadHandle) != _timerHasOwnership.end()) {
        retVal = _timerHasOwnership[timerThreadHandle];
    }
    lock.Unlock();

    return retVal;
}

OSTimer::OSTimer(qcc::Timer* timer) : _timer(timer)
{
}

void OSTimer::AllocThreadState()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    _timer->lock.Lock();
    _timerHasOwnership[timerThreadHandle] = true;
    _timer->lock.Unlock();
}

void OSTimer::DeleteThreadState()
{
    void* timerThreadHandle = reinterpret_cast<void*>(Thread::GetThread());
    _timer->lock.Lock();
    if (_timerHasOwnership.find(timerThreadHandle) != _timerHasOwnership.end()) {
        _timerHasOwnership.erase(timerThreadHandle);
    }
    _timer->lock.Unlock();
}

void OSTimer::MarshalThreadState(void* srcThread, void* destThread)
{
    _timer->lock.Lock();
    if (_timerHasOwnership.find(srcThread) != _timerHasOwnership.end()) {
        _timerHasOwnership[destThread] = _timerHasOwnership[srcThread];
    }
    _timer->lock.Unlock();
}

void OSTimer::TimerCallback(Windows::System::Threading::ThreadPoolTimer ^ timer)
{
    _timer->TimerCallback((void*)timer);
}

OSAlarm::OSAlarm() : _timer(nullptr)
{
}

}
