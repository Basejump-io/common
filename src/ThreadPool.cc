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

#include <assert.h>
#include <qcc/ThreadPool.h>

#define QCC_MODULE "THREADPOOL"

namespace qcc {

void Runnable::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    QCC_DbgPrintf(("Runnable::AlarmTriggered()"));

    /*
     * Execute the user's provided run function.  Note that this is done in an
     * OS-specific context.  For example, on Linux-based systems, this may
     * happen in the context of a pthread from the Timer; but on Windows systems
     * it may happen in the context of an asynchronous OS callback.
     */
    Run();

    /*
     * Tell the threadpool that we are done with this runnable object.  This
     * allows the threadpool to release its reference to the object which may
     * result in an immediate delete of the object.  So one must never refer to
     * the underlying runnable after this point.
     */
    m_threadpool->Release(this);
}

ThreadPool::ThreadPool(const char* name, uint32_t poolsize)
    : m_stopping(false), m_poolsize(poolsize), m_dispatcher(name, false, poolsize)
{
    QCC_DbgPrintf(("ThreadPool::ThreadPool()"));

    assert(poolsize && "ThreadPool::ThreadPool(): Empty pools are no good for anyone");

    /*
     * Start the dispatcher Timer.  The Timer is the code that will cause alarms
     * to be executed.  It is the alarms that provide the callbacks that the
     * timer will fire.  Our timer will have a concurrency variable that
     * corresponds to the poolsize provided.  This means that there will be
     * <poolsize> theads waiting to dispatch expired alarms (either AllJoyn
     * created in Posix systems or OS dispatched in Windows systems).  The
     * concurrent threads in the Timer provide us with the thread pool we
     * actually use.
     */
    m_dispatcher.Start();

    /*
     * Set the event that callers will ultimately use to sleep on until a thread
     * becomes available.  We just created a Timer dispatcher which has
     * concurrency of at least one, so there is definitely a thread available.
     */
    m_event.SetEvent();
}

ThreadPool::~ThreadPool()
{
    QCC_DbgPrintf(("ThreadPool::~ThreadPool(): %d closures remain", m_closures.size()));
    Stop();
    Join();

    /*
     * We have joined the underlying timer, so all of its threads must be
     * stopped.  That doesn't necessarily mean that they have executed the
     * AlarmTriggered() function that would take the closure off of the
     * pending closures list.  In this case, we need to make sure we clear
     * the closures map so we release the underlying Ptr and free the
     * runnable.
     */
    m_closures.clear();
}

QStatus ThreadPool::Stop()
{
    QCC_DbgPrintf(("ThreadPool::Stop()"));
    m_stopping = true;
    QStatus status = m_dispatcher.Stop();
    return status;

}

QStatus ThreadPool::Join()
{
    QCC_DbgPrintf(("ThreadPool::Join()"));
    assert(m_stopping && "ThreadPool::Join(): must have previously Stop()ped");
    QStatus status = m_dispatcher.Join();
    return status;
}

/*
 * Convenience function to get the number of threads currently executing or
 * waiting to execute.  This is different than GetConcurrency() which returns
 * the number of threads in the thread pool (which may or may not be currently
 * executing or waiting to execute.
 */
uint32_t ThreadPool::GetN(void)
{
    QCC_DbgPrintf(("ThreadPool::GetN()"));

    uint32_t n = 0;
    m_lock.Lock();
    n = m_closures.size();
    m_lock.Unlock();
    return n;
}

QStatus ThreadPool::Execute(Ptr<Runnable> runnable)
{
    QCC_DbgPrintf(("ThreadPool::Execute()"));

    m_lock.Lock();

    /*
     * Refuse to add any new closures if we're in the process of closing.
     */
    if (m_stopping) {
        m_lock.Unlock();
        QCC_DbgPrintf(("ThreadPool::Execute(): Stopping"));
        return ER_THREADPOOL_STOPPING;
    }

    /*
     * Since AllJoyn is at its heart a distributed network application, and what
     * drives the execution of our threads will be network traffic, we need to
     * be able to apply backpressure to the network to avoid exhausting all
     * available resources.  This is enabled by returning an error when all of
     * the threads are in process.  This is a thread pool, not a work queue.
     */
    if (m_closures.size() == m_poolsize) {
        m_lock.Unlock();
        QCC_DbgPrintf(("ThreadPool::Execute(): Exhausted"));
        return ER_THREADPOOL_EXHAUSTED;
    }

    /*
     * We need to make sure that the runnable object is kept alive while it is
     * waiting to be run (and while it is running) so we keep a reference to it
     * until we don't need it any more.  This also allows us to keep track of
     * how many pending operations there are.  Note that the map is keyed on the
     * underlying runnable object pointer, but the smart pointer is saved in the
     * map in order to hold a reference to the object.
     */
    QCC_DbgPrintf(("ThreadPool::Execute(): Schedule runnable"));
    m_closures[runnable.Peek()] = runnable;
    QCC_DbgPrintf(("ThreadPool::Execute(): %d closures remain", m_closures.size()));

    /*
     * Tell the runnable object where to contact us when it is done executing.
     * Since we're not the one that is actually going to dispatch the thread
     * we need to have whatever thread that goes in there call us us back so
     * we can know when to release the object.
     */
    runnable->SetThreadPool(this);

    /*
     * The trick here is to add an alarm that expires immediately and executes
     * the AlarmTriggered method of the provided Runnable object.  This will
     * call the Run() method of the Runnable.  So although we use Timers and
     * Alarms, we schedule everthing to happen immediately, and the result looks
     * like a thread pool that we all know and love.
     *
     * Since <runnable> is a smart pointer, we need to get the underlying object
     * pointer using Peek().  This object pointer is automagically cast to an
     * AlarmListener which defines which actual callback which the alarm will
     * call.
     */
    Alarm alarm = Alarm(0, runnable.Peek(), 0, NULL);
    QCC_DbgPrintf(("ThreadPool::Execute(): AddAlarm()"));
    QStatus status = m_dispatcher.AddAlarm(alarm);
    m_lock.Unlock();
    return status;
}

void ThreadPool::Release(Runnable* runnable)
{
    QCC_DbgPrintf(("ThreadPool::Release()"));

    /*
     * After the closure corresponding to the Runnable pointer above has finished
     * executing, it needs to call us back here and tell us that its heap object is
     * no longer required; and that we can release our hold on that object.
     *
     * We save a smart pointer to the Runnable in our m_closures map, so we need to
     * erase that entry.  Since the entry is a smart pointer, the act of erasing it
     * will cause the reference count of the object to be decremented and the
     * underlying object to be deleted if possible.
     */
    m_lock.Lock();
    RunnableEntry::iterator i = m_closures.find(runnable);
    assert(i != m_closures.end() && "ThreadPool::Release(): Cannot find closure to release");
    m_closures.erase(i);

    /*
     * Release needs to work in conjunction with Execute() and
     * WaitForAvailableThread() to ensure that no than m_poolSize threads are
     * dispatched at any one time.  We set an event every time a thread
     * completes its Run() method which can then wake up an external thread
     * waiting for an available thread to do its work.
     */
    m_event.SetEvent();

    QCC_DbgPrintf(("ThreadPool::Release(): %d closures remain", m_closures.size()));

    m_lock.Unlock();
}

QStatus ThreadPool::WaitForAvailableThread(void)
{
    QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread()"));

    /*
     * This method needs to work in conjunction with Execute() and Release() to
     * ensure that no more than m_poolSize threads are dispatched at any one
     * time.
     */
    m_lock.Lock();

    if (m_stopping) {
        m_lock.Unlock();
        QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Stopping"));
        return ER_THREADPOOL_STOPPING;
    }

    bool threadAvailable = m_closures.size() < m_poolsize;

    /*
     * There is a closure on the m_closures map for the duration of time when a
     * Runnable closure is waiting to be executed and in process.  So, if the
     * size of the m_closures map is less than the pool size, it means that not
     * all of our thread pool threads are occupied and at least one is
     * available, so we just return.
     */
    if (threadAvailable) {
        m_lock.Unlock();
        QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Thread available"));
        return ER_OK;
    }

    /*
     * We know that all of our threads are occupied, so we have to wait until
     * one of the Runnable closures is executed.  At the end of the process,
     * Release() will be called.  This will remove a closure from the m_closures
     * map and set m_event.  Our job here is then to wait until the event is
     * set and then look around to see if we should return.  We should only
     * return if there's a thread available to avoid the thundering herd.
     */
    for (;;) {
        m_lock.Unlock();
        QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Waiting on thread completion event"));

        /*
         * We are executing in the context of some unknown (to us) thread.  This
         * thread can be stopped and alerted using its own mechanisms so we have
         * to play fair with all of that.  The only return code we are interested
         * in is ER_OK, which means that we set our event.  Any other error code
         * should be returned to the caller, who can figure out the right thing
         * to do.
         */
        QStatus status = Event::Wait(m_event, Event::WAIT_FOREVER);
        if (status != ER_OK) {
            QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Event::Wait() error"));
            return status;
        }

        QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Got thread completion event"));
        m_lock.Lock();

        /*
         * Only allow one thead to return per Release() (thread completion)
         * event.
         */
        m_event.ResetEvent();

        /*
         * We can't have an available thread if we're stopping.
         */
        if (m_stopping) {
            m_lock.Unlock();
            QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Stopping"));
            return ER_THREADPOOL_STOPPING;
        }

        /*
         * If there's an available thread, return and allow the calling thread
         * to proceed with its presumable Execute().  If there's mo available
         * thread, we need to Event::Wait() again and wait for one.  Since
         * there's one event, it is possible that multiple waiting threads are
         * awakened when a thread completes.
         */
        if (m_closures.size() < m_poolsize) {
            m_lock.Unlock();
            QCC_DbgPrintf(("ThreadPool::WaitForAvailableThread(): Thread available"));
            return ER_OK;
        }
    }

    m_lock.Unlock();
    assert(false && "ThreadPool::WaitForAvailableThread(): Can't happen");
    return ER_FAIL;
}

} // namespace qcc
