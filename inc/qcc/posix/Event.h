/**
 * @file
 *
 * Platform independent event implementation
 */

/******************************************************************************
 *
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

#ifndef _OS_QCC_EVENT_H
#define _OS_QCC_EVENT_H

#include <qcc/platform.h>

#include <vector>

#include <qcc/atomic.h>

#include <Status.h>


/** @internal */
namespace qcc {

/** @internal Forward Reference */
class Source;

/**
 * Events are used to send signals between threads.
 */
class Event {
  public:

    /** Cause Wait to have no timeout */
    static const uint32_t WAIT_FOREVER = static_cast<uint32_t>(-1);

    /** Singleton always set Event */
    static Event alwaysSet;

    /** Singleton never set Event */
    static Event neverSet;

    /** Type of event */
    typedef enum {
        GEN_PURPOSE,     /**< General purpose pipe backed event */
        IO_READ,         /**< IO Read event */
        IO_WRITE,        /**< IO Write event */
        TIMED            /**< Event fires automatically when limit time is reached */
    } EventType;

    /** Create a general purpose Event. */
    Event();

    /**
     * Create a timed event.
     * Timed events cannot be manually set and reset.
     *
     * @param delay    Number of milliseconds to delay before Event is automatically set.
     * @param period   Number of milliseconds between auto-set events or 0 to indicate no repeat.
     */
    Event(uint32_t delay, uint32_t period = 0);

    /**
     * Constructor used by Linux specific I/O sources/sinks
     * (This constructor should only be used within Linux platform specific code.)
     *
     * @param event       Event whose underlying file descriptor is used as basis for new event
     * @param eventType   Type of event.
     * @param genPurpose  true if event should act as both an I/O event and a gen purpose event.
     */
    Event(Event& event, EventType eventType, bool genPurpose);

    /**
     * Constructor used by Linux specific I/O sources/sinks
     * (This constructor should only be used within Linux platform specific code.)
     *
     * @param ioFd        I/O file descriptor associated with this event.
     * @param genPurpose  true if event should act as both an I/O event and a gen purpose event.
     */
    Event(int ioFd, EventType eventType, bool genPurpose);

    /** Destructor */
    ~Event();

    /**
     * Wait on a group of events.
     * The call to Wait will return when any of the events on the list is signaled.
     *
     * @param checkEvents    Vector of event object references to wait on.
     * @param signaledEvents Vector of event object references from checkEvents that are signaled.
     * @param maxMs          Max number of milliseconds to wait or WAIT_FOREVER to wait forever.
     * @return ER_OK if successful.
     */
    static QStatus Wait(const std::vector<Event*>& checkEvents,
                        std::vector<Event*>& signaledEvents,
                        uint32_t maxMs = WAIT_FOREVER);

    /**
     * Wait on a single event.
     * The call to Wait will return when the event is signaled.
     *
     * @param event   Event to wait on.
     * @param maxMs   Max number of milliseconds to wait or WAIT_FOREVER to wait forever.
     * @return ER_OK if successful.
     */
    static QStatus Wait(Event& event, uint32_t maxMs = WAIT_FOREVER);


    /**
     * Set the event to the signaled state.
     * All threads that are waiting on the event will become runnable.
     * Calling SetEvent when the state is already signaled has no effect.
     *
     * @return ER_OK if successful.
     */
    QStatus SetEvent();

    /**
     * Reset the event to the non-signaled state.
     * Threads that call wait() will block until the event state becomes signaled.
     * Calling ResetEvent() when the state of the event is non-signaled has no effect.
     *
     * @return ER_OK if successful.
     */
    QStatus ResetEvent();

    /**
     * Indicate whether the event is currently signaled.
     *
     * @return true iff event is signaled.
     */
    bool IsSet();

    /**
     * Reset TIMED event and set next auto-set delay and period.
     *
     * @param delay    Number of milliseconds to delay before Event is automatically set.
     * @param period   Number of milliseconds between auto-set events or 0 to indicate no repeat.
     */
    void ResetTime(uint32_t delay, uint32_t period);

    /**
     * Replace I/O event source with a new one.
     *
     * @param event    Dependent event.
     */
    void ReplaceIO(Event& event);

    /**
     * Get the underlying file descriptor for general purpose and I/O events.
     * This returns -1 if there is no underlying file descriptor.  Use of this
     * function is not portable and should only be used in platform specific
     * code.
     *
     * @return  The underlying file descriptor or -1.
     */
    int GetFD() { return (fd == -1) ? ioFd : fd; }

    /**
     * Get the number of threads that are currently blocked waiting for this event
     *
     * @return The number of blocked threads
     */
    uint32_t GetNumBlockedThreads() { return numThreads; }

  private:

    int fd;                 /**< File descriptor linked to general purpose event or -1 */
    int signalFd;           /**< File descriptor used by GEN_PURPOSE events to manually set/reset event */
    int ioFd;               /**< I/O File descriptor associated with event or -1 */
    EventType eventType;    /**< Indicates type of event */
    uint32_t timestamp;     /**< time for next triggering of TIMED Event */
    uint32_t period;        /**< Number of milliseconds between periodic timed events */
    int32_t numThreads;     /**< Number of threads currently waiting on this event */

    /**
     * Protected copy constructor.
     * Events on some platforms (windows) cannot be safely copied because they contain events handles.
     */
    Event& operator=(const Event& evt) { return *this; }

    /**
     * Increment the count of threads blocked on this event
     */
    void IncrementNumThreads() { IncrementAndFetch(&numThreads); }

    /**
     * Decrement the count of threads blocked on this event
     */
    void DecrementNumThreads() { DecrementAndFetch(&numThreads); }

};

}  /* namespace */

#endif
