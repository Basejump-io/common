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
#include <Status.h>

#define QCC_MODULE  "TIMER"

#define WORKER_IDLE_TIMEOUT_MS  20
#define FALLBEHIND_WARNING_MS   500

using namespace std;
using namespace qcc;

namespace qcc {

class TimerThread : public Thread {
  public:

    enum {
        STOPPED,    /**< Thread must be started via Start() */
        STARTING,   /**< Thread has been Started but is not ready to service requests */
        IDLE,       /**< Thrad is sleeping. Waiting to be alerted via Alert() */
        RUNNING,    /**< Thread is servicing an AlarmTriggered callback */
        STOPPING    /**< Thread is stopping due to extended idle time. Not ready for Start or Alert */
    } state;

    TimerThread(const String& name, int index, Timer* timer)
        : Thread(name),
        state(STOPPED),
        index(index),
        timer(timer),
        currentAlarm(NULL) { }

    QStatus Start(void* arg, ThreadListener* listener);

    const Alarm* GetCurrentAlarm() const { return currentAlarm; }

    int GetIndex() const { return index; }

  protected:
    virtual ThreadReturn STDCALL Run(void* arg);

  private:
    const int index;
    Timer* timer;
    const Alarm* currentAlarm;
};

}

Timer::Timer(const char* name, bool expireOnExit, uint32_t concurency)
    : expireOnExit(expireOnExit), concurency(concurency), timerThreads(concurency), isRunning(false), controllerIdx(0)
{
    String nameStr = name;
    for (uint32_t i = 0; i < concurency; ++i) {
        timerThreads[i] = new TimerThread(nameStr, i, this);
    }
}

Timer::~Timer()
{
    Stop();
    Join();
    for (uint32_t i = 0; i < concurency; ++i) {
        delete timerThreads[i];
        timerThreads[i] = NULL;
    }
}

QStatus Timer::Start()
{
    QStatus status = ER_OK;
    lock.Lock();
    if (!isRunning) {
        controllerIdx = 0;
        isRunning = true;
        status = timerThreads[0]->Start(NULL, this);
        isRunning = false;
        if (status == ER_OK) {
            uint64_t startTs = GetTimestamp64();
            while (timerThreads[0]->state != TimerThread::IDLE) {
                if ((startTs + 5000) < GetTimestamp64()) {
                    status = ER_FAIL;
                    break;
                } else {
                    lock.Unlock();
                    Sleep(2);
                    lock.Lock();
                }
            }
        }
        isRunning = (status == ER_OK);
    }
    lock.Unlock();
    return status;
}

QStatus Timer::Stop()
{
    QStatus status = ER_OK;
    lock.Lock();
    isRunning = false;
    lock.Unlock();
    for (size_t i = 0; i < timerThreads.size(); ++i) {
        QStatus tStatus = timerThreads[i]->Stop();
        status = (status == ER_OK) ? tStatus : status;
    }
    return status;
}

QStatus Timer::Join()
{
    QStatus status = ER_OK;
    for (size_t i = 0; i < timerThreads.size(); ++i) {
        QStatus tStatus = timerThreads[i]->Join();
        status = (status == ER_OK) ? tStatus : status;
    }
    return status;
}

QStatus Timer::AddAlarm(const Alarm& alarm)
{
    QStatus status = ER_OK;
    lock.Lock();
    if (isRunning) {
        bool alertThread = alarms.empty() || (alarm < *alarms.begin());
        alarms.insert(alarm);

        QStatus status = ER_OK;
        if (alertThread && (controllerIdx >= 0)) {
            TimerThread* tt = timerThreads[controllerIdx];
            if (tt->state == TimerThread::IDLE) {
                status = tt->Alert();
            }
        }
    } else {
        status = ER_TIMER_EXITING;
    }
    lock.Unlock();
    return status;
}

void Timer::RemoveAlarm(const Alarm& alarm, bool blockIfTriggered)
{
    lock.Lock();
    if (isRunning) {
        multiset<Alarm>::iterator it = alarms.find(alarm);
        if (it != alarms.end()) {
            alarms.erase(it);
        } else if (blockIfTriggered) {
            /*
             * There might be a call in progress to the alarm that is being removed.
             * RemoveAlarm must not return until this alarm is finished.
             */
            for (size_t i = 0; i < concurency; ++i) {
                if (timerThreads[i] == Thread::GetThread()) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (isRunning && curAlarm && (*curAlarm == alarm)) {
                    lock.Unlock();
                    qcc::Sleep(2);
                    lock.Lock();
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
}

QStatus Timer::ReplaceAlarm(const Alarm& origAlarm, const Alarm& newAlarm, bool blockIfTriggered)
{
    QStatus status = ER_NO_SUCH_ALARM;
    lock.Lock();
    if (isRunning) {
        multiset<Alarm>::iterator it = alarms.find(origAlarm);
        if (it != alarms.end()) {
            alarms.erase(it);
            status = AddAlarm(newAlarm);
        } else if (blockIfTriggered) {
            /*
             * There might be a call in progress to origAlarm.
             * RemoveAlarm must not return until this alarm is finished.
             */
            for (size_t i = 0; i < concurency; ++i) {
                if (timerThreads[i] == Thread::GetThread()) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (isRunning && curAlarm && (*curAlarm == origAlarm)) {
                    lock.Unlock();
                    qcc::Sleep(2);
                    lock.Lock();
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return status;
}

bool Timer::RemoveAlarm(const AlarmListener& listener, Alarm& alarm)
{
    bool removedOne = false;
    lock.Lock();
    if (isRunning) {
        for (multiset<Alarm>::iterator it = alarms.begin(); it != alarms.end(); ++it) {
            if (it->listener == &listener) {
                alarms.erase(it);
                removedOne = true;
                break;
            }
        }
        /*
         * This function is most likely being called because the listener is about to be freed. If there
         * are no alarms remaining check that we are not currently servicing an alarm for this listener.
         * If we are, wait until the listener returns.
         */
        if (!removedOne) {
            for (size_t i = 0; i < concurency; ++i) {
                if (timerThreads[i] == Thread::GetThread()) {
                    continue;
                }
                const Alarm* curAlarm = timerThreads[i]->GetCurrentAlarm();
                while (isRunning && curAlarm && (curAlarm->listener == &listener)) {
                    lock.Unlock();
                    qcc::Sleep(5);
                    lock.Lock();
                    curAlarm = timerThreads[i]->GetCurrentAlarm();
                }
            }
        }
    }
    lock.Unlock();
    return removedOne;
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

QStatus TimerThread::Start(void* arg, ThreadListener* listener)
{
    QStatus status = ER_OK;
    timer->lock.Lock();
    if (timer->isRunning) {
        status = Thread::Start(arg, listener);
        state = TimerThread::STARTING;
    }
    timer->lock.Unlock();
    return status;
}

ThreadReturn STDCALL TimerThread::Run(void* arg)
{
    /* Wait for first entry on (sorted) alarm list to expire */
    timer->lock.Lock();
    while (!IsStopping()) {
        Timespec now;
        GetTimeNow(&now);
        bool isController = (timer->controllerIdx == index);
        if (!isController && (timer->controllerIdx == -1)) {
            if (timer->yieldControllerTime.GetAbsoluteMillis() && ((now - timer->yieldControllerTime) > FALLBEHIND_WARNING_MS)) {
                QCC_LogError(ER_TIMER_FALLBEHIND, ("Timer has fallen behind by %ld ms", now - timer->yieldControllerTime));
            }
            timer->controllerIdx = index;
            isController = true;
        }
        if (!timer->alarms.empty()) {
            const Alarm& topAlarm = *(timer->alarms.begin());
            int64_t delay = topAlarm.alarmTime - now;
            if ((delay > 0) && (isController || (delay < WORKER_IDLE_TIMEOUT_MS))) {
                state = IDLE;
                timer->lock.Unlock();
                Event evt(static_cast<uint32_t>(delay), 0);
                Event::Wait(evt);
                stopEvent.ResetEvent();
            } else if (isController || (delay <= 0)) {
                TimerThread* tt = NULL;
                if (isController) {
                    /* Look for an idle or stopped worker to execute alarm callback for us */
                    for (size_t i = 1; i < timer->concurency; ++i) {
                        if (i != static_cast<size_t>(index)) {
                            if (timer->timerThreads[i]->state == TimerThread::IDLE) {
                                tt = timer->timerThreads[i];
                                break;
                            } else if (timer->timerThreads[i]->state == TimerThread::STOPPED) {
                                tt = timer->timerThreads[i];
                            }
                        }
                    }
                    if (tt) {
                        if (tt->state == TimerThread::IDLE) {
                            QStatus status = tt->Alert();
                            if (status != ER_OK) {
                                QCC_LogError(status, ("Error alerting timer thread %s", tt->GetName().c_str()));
                            }
                        } else if (tt->state == TimerThread::STOPPED) {
                            QStatus status = tt->Start(NULL, timer);
                            if (status != ER_OK) {
                                QCC_LogError(status, ("Error starting timer thread %s", tt->GetName().c_str()));
                            }
                        }
                    }
                }
                if (!tt) {
                    multiset<Alarm>::iterator it = timer->alarms.begin();
                    Alarm top = *it;
                    timer->alarms.erase(it);
                    currentAlarm = &top;
                    state = RUNNING;
                    if (isController) {
                        timer->controllerIdx = -1;
                        GetTimeNow(&timer->yieldControllerTime);
                        isController = false;
                    }
                    timer->lock.Unlock();
                    stopEvent.ResetEvent();
                    (top.listener->AlarmTriggered)(top, ER_OK);
                    currentAlarm = NULL;
                    if (0 != top.periodMs) {
                        top.alarmTime += top.periodMs;
                        if (top.alarmTime < now) {
                            top.alarmTime = now;
                        }
                        timer->AddAlarm(top);
                    }
                } else {
                    timer->lock.Unlock();
                }

            } else {
                /* Worker with nothing to do */
                state = STOPPING;
                break;
            }
        } else {
            /* Alarm list is empty */
            if (isController) {
                state = IDLE;
                timer->lock.Unlock();
                Event evt(Event::WAIT_FOREVER, 0);
                Event::Wait(evt);
                stopEvent.ResetEvent();
            } else {
                state = STOPPING;
                break;
            }
        }
        timer->lock.Lock();
    }
    state = STOPPING;
    timer->lock.Unlock();
    return (ThreadReturn) 0;
}

void Timer::ThreadExit(Thread* thread)
{
    TimerThread* tt = static_cast<TimerThread*>(thread);
    lock.Lock();
    if ((tt->GetIndex() == controllerIdx) && expireOnExit) {
        /* Call all alarms */
        while (!alarms.empty()) {
            /*
             * Note it is possible that the callback will call RemoveAlarm()
             */
            multiset<Alarm>::iterator it = alarms.begin();
            Alarm alarm = *it;
            alarms.erase(it);
            lock.Unlock();
            alarm.listener->AlarmTriggered(alarm, ER_TIMER_EXITING);
            lock.Lock();
        }
    }
    tt->state = TimerThread::STOPPED;
    lock.Unlock();
    tt->Join();
}
