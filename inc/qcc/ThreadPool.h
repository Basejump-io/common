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

/**
 * A class in the spirit of the Java Runnable object that is used to define
 * an object which is executable by a ThreadPool.
 *
 * In order to ask the ThreadPool to execute a task, inherit from the
 * Runnable class and provide a Run() method.  You must allocate a Runnable
 * object on the heap and then call ThreadPool::Execute() providing a pointer
 * to the Runnable object.
 *
 * Once the Run() method has executed, the runnable will be automatically
 * deleted.
 */
class Runnable : private qcc::AlarmListener {
public:
    /**
     * This method is called by the ThreadPool when the Runnable object is
     * dispatched to a thread.  A client of the thread pool is expected to
     * define a method in a derived class that does useful work when Run()
     * is called.
     */
    virtual void Run(void) = 0;

private:
    /**
     * ThreadPool must be a friend in order to make AlarmTriggered an
     * accessible base class method of a user's Runnable.
     */
    friend class ThreadPool;

    /**
     * AlarmTriggered is the method that is called to dispatch an alarm.
     * Our Run() methods are driven by alarm expirations that happen to
     * always occur immediately.
     *
     * Note that the enclosing runnable object is automatically deleted
     * afte the Run method is executed.
     */
    virtual void AlarmTriggered(const Alarm& alarm, QStatus reason)
    {
        Run();
        delete this;
    }
};

/**
 * A class in the spirit of the Java ThreadPoolExecutor object that is used
 * to provide a simple way to execute tasks in the context of a separate
 * thread.
 *
 * In order to ask a ThreadPool to execute a task, one must inherit from the
 * Runnable class and provide a Run() method.
 */
class ThreadPool {
public:
    /**
     * Construct a thread pool with a given name and pool size.
     *
     * @param name     The name of the thread pool (used in logging).
     * @param poolSize The number of threads available in the pool.
     */
    ThreadPool(const char* name, uint32_t poolSize) : dispatcher(name, false, poolSize)
    {
        /*
         * Start the dispatcher timer.  This timer will have a concurrency
         * corresponding to the poolSize provided.  This means that there will
         * be <poolSize> theads waiting to dispatch expired alarms.  The
         * concurrent threads in the Timer provide us with our thread pool.
         */
        dispatcher.Start();
    }

    /**
     * Destroy a thread pool.
     */
    virtual ~ThreadPool()
    {
        /*
         * Send a message to the timer requesting that it stop all of its
         * theads.
         */
        dispatcher.Stop();

        /*
         * Wait for all of the threads in our associated timer to exit.  Once
         * this happens, it is safe for us to finish tearing down our object.
         * Note that this call can block or a time limited only by the execution
         * time of the threads dispatched.
         */
        dispatcher.Join();
    }

    /**
     * Execute a Runnable task one one of the threads of the thread pool.
     *
     * The execute method takes a pointer to a Runnable object.  This object
     * acts as a closure which essentialy is a pre-packaged function call.  The
     * function is called Run() in this case.  Since we need to keep the package
     * around until Run() actually executes, and the thread that calls Execute()
     * may be long gone by the time this happens, the Runnable must be allocated
     * on the heap.  Only the thread pool knows when the Runnable is no longer
     * needed, so it takes responsibility for managing the memory of the runnable
     * when Execute is called.
     *
     * Each call to Execute must provide a pointer to a unique Runnable 
     *
     * @param runnable A pointer to a Runnable object providing the Run() method
     *                 which one of the threads in this thread pool will execute.
     */
    void Execute(Runnable* runnable) 
    {
        /*
         * The trick here is to add an alarm that expires immediately and
         * executes the AlarmTriggered method of the provided Runnable
         * object.  This will call the Run() method of the Runnable.  So
         * although we use Timers and Alarms, we schedule everthing to
         * happen immediately, and the result looks like a thread pool
         * that we all know and love.
         */
        qcc::Alarm alarm = qcc::Alarm(0, runnable, 0, NULL);
        dispatcher.AddAlarm(alarm);
    }

private:
    /**
     * Assignment operator is private - ThreadPools cannot be assigned.
     */
    ThreadPool& operator=(const ThreadPool& other);

    /**
     * Copy constructor is private - ThreadPools cannot be copied.
     */
    ThreadPool(const ThreadPool& other);

    qcc::Timer dispatcher;
};

} // namespace qcc

#endif
