/**
 * @file
 *
 * Timer thread
 */

/******************************************************************************
 * $Revision: 14/5 $
 *
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
#ifndef _QCC_TIMER_H
#define _QCC_TIMER_H

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <set>

#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

namespace qcc {

/** @internal Forward declaration */
class Timer;
class Alarm;

/**
 * An alarm listener is capable of receiving alarm callbacks
 */
class AlarmListener {
    friend class Timer;

  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~AlarmListener() { }

  private:
    virtual void AlarmTriggered(const Alarm& alarm) = 0;
};

class Alarm {
    friend class Timer;

  public:

    /** Disable timeout value */
    static const uint32_t WAIT_FOREVER = static_cast<uint32_t>(-1);

    /**
     * Create a default (unusable) alarm.
     */
    Alarm() : listener(NULL), periodMs(0), context(NULL) { }

    /**
     * Create an alarm that can be added to a Timer.
     *
     * @param absoluteTime    Alarm time.
     * @param listener        Object to call when alarm is triggered.
     * @param periodMs        Periodicity of alarm in ms or 0 for no repeat.
     * @param context         Opaque context passed to listener callback.
     */
    Alarm(Timespec absoluteTime, AlarmListener* listener, uint32_t periodMs = 0, void* context = NULL)
        : alarmTime(absoluteTime), listener(listener), periodMs(periodMs), context(context) { }

    /**
     * Create an alarm that can be added to a Timer.
     *
     * @param relativeTimed   Number of ms from now that alarm will trigger.
     * @param listener        Object to call when alarm is triggered.
     * @param periodMs        Periodicity of alarm in ms or 0 for no repeat.
     * @param context         Opaque context passed to listener callback.
     */
    Alarm(uint32_t relativeTime, AlarmListener* listener, uint32_t periodMs = 0, void* context = NULL)
        : alarmTime(), listener(listener), periodMs(periodMs), context(context)
    {
        if (relativeTime == WAIT_FOREVER) {
            alarmTime = END_OF_TIME;
        } else {
            GetTimeNow(&alarmTime);
            alarmTime += relativeTime;
        }
    }

    /**
     * Get context associated with alarm.
     *
     * @return User defined context.
     */
    void* GetContext(void) const { return context; }

    /**
     * Get the absolute alarmTime in milliseconds
     */
    uint64_t GetAlarmTime() const { return alarmTime.GetAbsoluteMillis(); }

    /**
     * Return true if this Alarm's time is less than the passed in alarm's time
     */
    bool operator<(const Alarm& other) const
    {
        return ((alarmTime < other.alarmTime) ||
                ((alarmTime == other.alarmTime) && ((listener < other.listener) ||
                                                    ((listener == other.listener) && ((context < other.context) ||
                                                                                      ((context == other.context) && (periodMs < other.periodMs)))))));
    }

    /**
     * Return true if two alarm instances are equal.
     */
    bool operator==(const Alarm& other) const
    {
        return (alarmTime == other.alarmTime) && (listener == other.listener) && (context == other.context) && (periodMs == other.periodMs);
    }

  private:

    Timespec alarmTime;
    AlarmListener* listener;
    uint32_t periodMs;
    mutable void* context;
};


class Timer : public Thread {

  public:

    /**
     * Constructor
     *
     * @param name  Optional name for the thread.
     */
    Timer(const char* name = "timer") : Thread(name), inUserCallback(false) { }

    /**
     * Associate an alarm with a timer.
     *
     * @param alarm     Alarm to add.
     */
    void AddAlarm(const Alarm& alarm);

    /**
     * Disassociate an alarm from a timer.
     *
     * @param alarm     Alarm to remove.
     */
    void RemoveAlarm(const Alarm& alarm);

  protected:

    /**
     * Thread entry point.
     *
     * @param arg  Unused thread arg
     */
    ThreadReturn STDCALL Run(void* arg);

  private:

    Mutex lock;
    std::set<Alarm, std::less<Alarm> >  alarms;
    bool inUserCallback;
};

}

#endif
