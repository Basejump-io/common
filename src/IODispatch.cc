/**
 * @file
 *
 * IODispatch listens on a set of file descriptors and provides callbacks for read/write
 */

/******************************************************************************
 * Copyright 2012-2013, Qualcomm Innovation Center, Inc.
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
#include <qcc/IODispatch.h>
#define QCC_MODULE "IODISPATCH"

using namespace qcc;
using namespace std;
IODispatch::IODispatch(const char* name, uint32_t concurrency) :
    timer(name, true, concurrency, false, 10),
    reload(false),
    isRunning(false),
    numAlarmsInProgress(0),
    crit(0)
{

}
IODispatch::~IODispatch()
{
    reload = true;
    Stop();
    Join();

    /* All endpoints should have already been stopped and joined.
     * so, there should be no dispatch entries.
     * Just a sanity check.
     */
    assert(dispatchEntries.size() == 0);
}
QStatus IODispatch::Start()
{
    /* Start the timer thread */
    QStatus status = timer.Start();

    if (status != ER_OK) {
        timer.Stop();
        timer.Join();
        return status;
    } else {
        isRunning = true;
        /* Start the main thread */
        return Thread::Start();
    }
}

QStatus IODispatch::Stop()
{
    lock.Lock();
    isRunning = false;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
    Stream* stream;
    while (it != dispatchEntries.end()) {

        stream = it->first;
        lock.Unlock();
        StopStream(stream);
        lock.Lock();
        it = dispatchEntries.upper_bound(stream);
    }
    lock.Unlock();

    Thread::Stop();
    timer.Stop();
    return ER_OK;
}

QStatus IODispatch::Join()
{
    lock.Lock();

    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
    Stream* stream;
    while (it != dispatchEntries.end()) {
        stream = it->first;
        lock.Unlock();
        JoinStream(stream);
        lock.Lock();
        it = dispatchEntries.upper_bound(stream);
    }
    lock.Unlock();

    Thread::Join();
    timer.Join();
    return ER_OK;
}

QStatus IODispatch::StartStream(Stream* stream, IOReadListener* readListener, IOWriteListener* writeListener, IOExitListener* exitListener)
{
    QCC_DbgTrace(("StartStream %p", stream));

    /* Dont attempt to register a stream if the IODispatch is shutting down */
    if (!isRunning) {
        return ER_BUS_STOPPING;
    }
    lock.Lock();
    if (dispatchEntries.find(stream) != dispatchEntries.end()) {
        lock.Unlock();
        return ER_INVALID_STREAM;

    }
    dispatchEntries[stream] = IODispatchEntry(stream, readListener, writeListener, exitListener);
    dispatchEntries[stream].readCtxt = new CallbackContext(stream, IO_READ);
    dispatchEntries[stream].writeCtxt = new CallbackContext(stream, IO_WRITE);
    dispatchEntries[stream].timeoutCtxt = new CallbackContext(stream, IO_TIMEOUT);
    dispatchEntries[stream].exitCtxt = new CallbackContext(stream, IO_EXIT);

    /* Set reload to false and alert the IODispatch::Run thread */
    reload = false;
    lock.Unlock();

    Thread::Alert();
    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors since we are adding a new stream.
     */
    return ER_OK;
}


QStatus IODispatch::StopStream(Stream* stream) {
    lock.Lock();
    QCC_DbgTrace(("StopStream %p", stream));
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);

    /* Check if stream is still present in dispatchEntries. */
    if (it == dispatchEntries.end()) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    if (it->second.stopping_state == IO_STOPPED) {
        lock.Unlock();
        return ER_FAIL;
    }
    IODispatchEntry dispatchEntry = it->second;

    /* Disable further read and writes on this stream */
    it->second.stopping_state = IO_STOPPING;

    /* Set reload to false and alert the IODispatch::Run thread */
    reload = false;
    int when = 0;
    AlarmListener* listener = this;
    if (isRunning) {

        Thread::Alert();

        /* Wait until the IODispatch::Run thread reloads the set of check events */
        while (!reload && crit && isRunning) {
            lock.Unlock();
            Sleep(1);
            lock.Lock();
        }
    }
    if (!isRunning) {
        if (it->second.stopping_state == IO_STOPPING) {
            it->second.stopping_state = IO_STOPPED;
            Alarm exitAlarm = Alarm(when, listener, it->second.exitCtxt);
            lock.Unlock();
            timer.AddAlarm(exitAlarm);
            lock.Lock();
        }

    }
    lock.Unlock();
    return ER_OK;
}
QStatus IODispatch::JoinStream(Stream* stream) {
    lock.Lock();
    QCC_DbgTrace(("JoinStream %p", stream));

    /* Make sure the stream exists. It may not, if it has already been joined. */
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);
    while (it != dispatchEntries.end()) {
        lock.Unlock();
        qcc::Sleep(10);
        lock.Lock();
        it = dispatchEntries.find(stream);
    }
    lock.Unlock();
    return ER_OK;
}
void IODispatch::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    timer.EnableReentrancy();
    lock.Lock();


    /* Find the stream associated with this alarm */
    CallbackContext* ctxt = static_cast<CallbackContext*>(alarm->GetContext());
    Stream* stream = ctxt->stream;
    /* If IODispatch is being shut down, only service
     * exit alarms. Ignore read/write/timeout alarms
     */
    if (!isRunning && ctxt->type != IO_EXIT) {
        lock.Unlock();
        return;
    }
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);

    if (it == dispatchEntries.end() || (it->second.stopping_state && ctxt->type != IO_EXIT)) {
        lock.Unlock();
        return;
    }
    IODispatchEntry dispatchEntry = it->second;
    switch (ctxt->type) {
    case IO_TIMEOUT:
        if (alarm != it->second.linkTimeoutAlarm) {
            lock.Unlock();
            return;
        }
        IncrementAndFetch(&numAlarmsInProgress);
        lock.Unlock();

        /* Make the read callback */
        dispatchEntry.readListener->LinkTimeoutCallback(*stream);
        DecrementAndFetch(&numAlarmsInProgress);
        break;

    case IO_READ:

        timer.RemoveAlarm(it->second.linkTimeoutAlarm, false /*Non blocking */);
        IncrementAndFetch(&numAlarmsInProgress);
        lock.Unlock();
        if (dispatchEntry.readEnable) {
            /* Ensure read has not been disabled */
            dispatchEntry.readListener->ReadCallback(*stream);
        }
        DecrementAndFetch(&numAlarmsInProgress);
        break;

    case IO_WRITE:
        IncrementAndFetch(&numAlarmsInProgress);
        lock.Unlock();

        /* Make the write callback */
        if (dispatchEntry.writeEnable) {
            /* Ensure write has not been disabled */
            dispatchEntry.writeListener->WriteCallback(*stream);
        }
        DecrementAndFetch(&numAlarmsInProgress);
        break;

    case IO_EXIT:

        lock.Unlock();

        if (isRunning) {
            /* Timer is running. Remove any pending alarms */
            timer.RemoveAlarm(dispatchEntry.readAlarm, true /* blocking */);
            timer.RemoveAlarm(dispatchEntry.linkTimeoutAlarm, true /* blocking */);
            timer.RemoveAlarm(dispatchEntry.writeAlarm, true /* blocking */);
        }
        /* If IODispatch has been stopped between the check to isRunning and now,
         * RemoveAlarms may not have successfully removed the alarm.
         * In that case, wait for any alarms that are in progress to finish.
         */
        while (!isRunning && numAlarmsInProgress) {
            Sleep(2);
        }
        /* Make the exit callback */
        dispatchEntry.exitListener->ExitCallback();
        /* Find and erase the stream entry */
        lock.Lock();
        it = dispatchEntries.find(stream);
        if (it == dispatchEntries.end()) {
            lock.Unlock();
            return;
        }
        if (it->second.readCtxt) {
            delete it->second.readCtxt;
            it->second.readCtxt = NULL;
        }
        if (it->second.readCtxt) {
            delete it->second.writeCtxt;
            it->second.writeCtxt = NULL;
        }
        if (it->second.readCtxt) {
            delete it->second.exitCtxt;
            it->second.exitCtxt = NULL;
        }
        if (it->second.readCtxt) {
            delete it->second.timeoutCtxt;
            it->second.timeoutCtxt = NULL;
        }
        dispatchEntries.erase(it);
        lock.Unlock();
        break;

    default:
        break;

    }
}

ThreadReturn STDCALL IODispatch::Run(void* arg) {
    vector<qcc::Event*> checkEvents, signaledEvents;
    int32_t when =  0;
    AlarmListener* listener = this;


    while (!IsStopping()) {
        checkEvents.clear();
        signaledEvents.clear();
        /* Add the stop event to list of events to check for */
        checkEvents.push_back(&stopEvent);

        /* Set reload to true to indicate that this thread is not in the Event::Wait and is
         * reloading the set of source and sink events
         */
        lock.Lock();
        reload = true;
        map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
        while (it != dispatchEntries.end() && isRunning) {
            /* If read is enabled and not in progress, add the source event for the stream to the
             * set of check events
             */
            if (it->second.stopping_state != IO_RUNNING) {
                it++;
                continue;
            }
            if (it->second.readEnable && !it->second.readInProgress) {
                checkEvents.push_back(&it->first->GetSourceEvent());
            }
            /* If write is enabled and not in progress, add the sink event for the stream to the
             * set of check events
             */
            if (it->second.writeEnable && !it->second.writeInProgress) {
                checkEvents.push_back(&it->first->GetSinkEvent());
            }
            it++;
        }
        lock.Unlock();
        /* Wait for an event to occur */
        IncrementAndFetch(&crit);
        qcc::Event::Wait(checkEvents, signaledEvents);
        DecrementAndFetch(&crit);
        lock.Lock();
        reload = true;
        it = dispatchEntries.begin();
        /* Add exit alarms for any streams that are being stopped.
         */
        while (it != dispatchEntries.end()) {
            if (it->second.stopping_state == IO_STOPPING) {
                Stream* s = it->first;
                Alarm exitAlarm = Alarm(when, listener, it->second.exitCtxt);
                lock.Unlock();
                QStatus status = timer.AddAlarm(exitAlarm);
                lock.Lock();
                if (status == ER_OK) {
                    it = dispatchEntries.find(s);
                    if (it != dispatchEntries.end()) {
                        it->second.stopping_state = IO_STOPPED;
                    }
                }
                it = dispatchEntries.upper_bound(s);
                continue;
            }
            it++;
        }
        lock.Unlock();
        for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                /* This thread has been alerted or is being stopped. Will check the IsStopping()
                 * flag when the while condition is encountered
                 */
                stopEvent.ResetEvent();
                continue;
            } else {
                lock.Lock();
                map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
                while (it != dispatchEntries.end()) {

                    Stream* stream = it->first;

                    if (it->second.stopping_state == IO_RUNNING) {
                        if (&stream->GetSourceEvent() == *i) {

                            if (it->second.readEnable) {
                                /* If the source event for a particular stream has been signalled,
                                 * add a readAlarm to fire now, and set readInProgress to true.
                                 */
                                it->second.readInProgress = true;
                                it->second.readAlarm = Alarm(when, listener, it->second.readCtxt);
                                Alarm readAlarm = it->second.readAlarm;
                                lock.Unlock();
                                timer.AddAlarm(readAlarm);
                                lock.Lock();
                                break;
                            }
                        } else if (&stream->GetSinkEvent() == *i) {
                            if (it->second.writeEnable) {
                                /* If the sink event for a particular stream has been signalled,
                                 * add a writeAlarm to fire now, and set writeInProgress to true.
                                 */
                                it->second.writeInProgress = true;
                                it->second.writeAlarm = Alarm(when, listener, it->second.writeCtxt);
                                Alarm writeAlarm = it->second.writeAlarm;
                                lock.Unlock();
                                timer.AddAlarm(writeAlarm);
                                lock.Lock();
                                break;
                            }
                        }
                    }
                    it++;
                }
                lock.Unlock();
            }
        }
    }
    lock.Lock();
    /* Set isRunning flag and reload flag. */
    reload = true;
    QCC_DbgPrintf(("IODispatch::Run exiting"));
    lock.Unlock();

    return (ThreadReturn) 0;
}

QStatus IODispatch::EnableReadCallback(const Source* source)
{
    lock.Lock();
    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    it->second.readEnable = true;
    it->second.readInProgress = false;

    /* Add a link timeout alarm if necessary. */
    if (it->second.linkTimeout != 0) {
        uint32_t temp = it->second.linkTimeout;
        AlarmListener* listener = this;
        it->second.linkTimeoutAlarm = Alarm(temp, listener, it->second.timeoutCtxt);
        Alarm timeoutAlarm = it->second.linkTimeoutAlarm;
        lock.Unlock();
        timer.AddAlarm(timeoutAlarm);
    } else {
        lock.Unlock();
    }
    Thread::Alert();
    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors since we're enabling read.
     */
    return ER_OK;
}

QStatus IODispatch::DisableReadCallback(const Source* source)
{
    lock.Lock();
    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    it->second.readEnable = false;
    lock.Unlock();
    Thread::Alert();
    /* Wait until the IODispatch::Run thread reloads the set of check events
     * since we are disabling read
     */
    while (!reload && crit && isRunning) {
        Sleep(10);
    }
    return ER_OK;
}

QStatus IODispatch::EnableWriteCallbackNow(Sink* sink)
{

    lock.Lock();
    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    if (it->second.writeEnable) {
        lock.Unlock();
        return ER_OK;
    }
    it->second.writeEnable = true;
    it->second.writeInProgress = true;

    int32_t when = 0;
    AlarmListener* listener = this;

    /* Add a write alarm to fire now, there is data ready to be written */
    it->second.writeAlarm = Alarm(when, listener, it->second.writeCtxt);
    Alarm writeAlarm = it->second.writeAlarm;
    QStatus status = timer.AddAlarmNonBlocking(writeAlarm);
    if (status == ER_TIMER_FULL) {
        /* Since the timer is full, just alert the main thread, so that
         * it can add a write alarm for this stream.
         */
        it->second.writeInProgress = false;
        Thread::Alert();
    }
    lock.Unlock();
    return ER_OK;
}
QStatus IODispatch::EnableWriteCallback(Sink* sink)
{
    lock.Lock();
    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    it->second.writeEnable = true;
    it->second.writeInProgress = false;
    lock.Unlock();
    Thread::Alert();
    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors, since we are enabling write callback.
     */
    return ER_OK;
}
QStatus IODispatch::DisableWriteCallback(const Sink* sink)
{
    lock.Lock();
    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    it->second.writeEnable = false;

    lock.Unlock();
    Thread::Alert();
    /* Wait until the IODispatch::Run thread reloads the set of check events
     * since we are disabling write.
     */
    while (!reload && crit && isRunning) {
        Sleep(10);
    }
    return ER_OK;
}

QStatus IODispatch::SetLinkTimeout(const Source* source, uint32_t linkTimeout) {

    lock.Lock();
    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    timer.RemoveAlarm(it->second.linkTimeoutAlarm, false);
    it->second.linkTimeout = linkTimeout * 1000;                        /* seconds to ms */
    if (it->second.linkTimeout != 0) {
        uint32_t temp = it->second.linkTimeout;
        AlarmListener* listener = this;
        it->second.linkTimeoutAlarm = Alarm(temp, listener, it->second.timeoutCtxt);
        Alarm timeoutAlarm = it->second.linkTimeoutAlarm;
        lock.Unlock();
        timer.AddAlarm(timeoutAlarm);
    } else {
        lock.Unlock();
    }
    return ER_OK;
}

QStatus IODispatch::EnableTimeoutCallback(const Source* source) {
    lock.Lock();
    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    if (it->second.linkTimeout != 0) {
        uint32_t temp = it->second.linkTimeout;
        AlarmListener* listener = this;
        it->second.linkTimeoutAlarm = Alarm(temp, listener, it->second.timeoutCtxt);
        Alarm timeoutAlarm = it->second.linkTimeoutAlarm;
        lock.Unlock();
        timer.AddAlarm(timeoutAlarm);
    } else {
        lock.Unlock();
    }

    return ER_OK;
}
