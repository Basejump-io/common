/**
 * @file
 *
 * This file just wraps platform specific header files that define the thread
 * abstraction interface.
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
#ifndef _QCC_THREAD_H
#define _QCC_THREAD_H

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <Status.h>

#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/Thread.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/Thread.h>
#else
#error No OS GROUP defined.
#endif

namespace qcc {

typedef void* ThreadReturn;

/**
 * Put current thread to sleep for specified number of milliseconds.
 *
 * @param ms    Number of milliseconds to sleep.
 */
QStatus Sleep(uint32_t ms);

/** @internal */
class Thread;

/**
 * Callback interface used to notify of thread exit.
 */
class ThreadListener {
  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~ThreadListener() { }

    /**
     * Called when the thread is about to exit.
     * The underlying Thread instance is guaranteed to not be accessed once
     * this callback returns. This allows implementations to free the Thread
     * if desired.
     *
     * @param thread   Thread that has exited.
     */
    virtual void ThreadExit(Thread* thread) = 0;
};

/**
 * Abstract encapsulation of the os-specific threads.
 */
class Thread {

  public:

    /**
     * Find the Thread for this thread (or NULL if no such thread).
     *
     * @return  Thread for this thread. If current resource is
     *          not a Thread, NULL is returned.
     */
    static Thread* GetThread();

    /**
     * Get the function name of this thread.
     *
     * @return The thread function name.
     */
    static const char* GetThreadName() { return GetThread()->GetName().c_str(); }

    /**
     * Release and deallocate all threads that are marked as "external"
     */
    static void CleanExternalThreads();

    /**
     * Function declaration for thread entry point
     *
     * @param arg    Opaque argument passed to thread entry.
     * @return   Thread exit code
     */
    typedef ThreadReturn (STDCALL * ThreadFunction)(void* arg);

    /**
     * Construct a new thread.
     *
     * @param funcName    String representation of the function name (defaults to empty).
     * @param func        Entry point for new thread or NULL to use Run() as entry point.
     * @param isExternal  Create a "wrapper" Thread object for the calling thread rather
     *                    than creating an actual thread.
     */
    Thread(qcc::String funcName = "", ThreadFunction func = NULL, bool isExternal = false);

    /**
     * The destructor will kill the thread if it is running.
     */
    virtual ~Thread(void);

    /**
     * Call Run() in its own thread with 'arg' as its argument.
     * Passed in arguments that are pointers to memory must either
     * have their ownership passed to Run, or must remain allocated
     * for the duration of the thread.  If the memory pointed to by
     * 'arg' is to be accessed by more than one threading resource,
     * then it must be protected through the use of Mutex's.<p>
     *
     * Subclasses that override this method should call the base class
     * implementation of Start.
     *
     * @param arg        The one and only parameter that 'func' will be called with
     *                   (defaults to NULL).
     *
     * @param listener   Listener to be informed of Thread events (defaults to NULL).
     *
     * @return  Indication of whether creation of the thread succeeded or not.
     */
    virtual QStatus Start(void* arg = NULL, ThreadListener* listener = NULL);

    /**
     * Stop the thread.
     *
     * This method sets the thread's isStopping state to true and set's the thread's stopped event
     * to unblock any I/O. Stopping a thread using this method relies on the implementation of Run()
     * (or threadfunc) to periodically check the state by calling IsStopping().<p>
     *
     * To stop a thread asynchronously (and unsafely) use Thread::Kill<p>
     *
     * Subclasses that override this method should call the base class
     * implementation of Stop.
     *
     * @return  ER_OK if request was successful. This does not imply
     *          that Stop will successfully stop the thread.
     *
     * @see Thread::Kill
     */
    virtual QStatus Stop(void);

    /**
     * Abort the thead.
     *
     * This method should be avoided since it does not guarantee that the thread's
     * resources (mutexes, sockets, file descriptors) are released before the
     * threads exit.
     *
     * @return  Indication of whether the thread was killed or not.
     *
     * @see Thread::Stop()
     */
    virtual QStatus Kill(void);

    /**
     * Alert a thread by causing any pending call to Event::Wait() to unblock
     * This functionality is very similar to Stop(). The difference is that
     * the thread is not required to cleanup and exit when this funciton is called.
     * This version of Alert leaves the alertCode undisturbed.
     *
     * @return ER_OK if request was successful.
     */
    virtual QStatus Alert(void);

    /**
     * Alert a thread by causing any pending call to Event::Wait() to unblock
     * This functionality is very similar to Stop(). The difference is that
     * the thread is not required to cleanup and exit when this funciton is called.
     * This version of Alert sets the threads alertCode.
     *
     * @param alertCode  Optional context that can be passed to the alerted thread.
     * @return ER_OK if request was successful.
     */
    virtual QStatus Alert(uint32_t alertCode);

    /**
     * This function allows one thread to wait for the completion of another
     * thread.  Once a thread has terminated, it's exit value may be examined.
     * A thread must not "join" itself.
     *
     * @return  Indication of whether the join operation succeeded or not.
     */
    virtual QStatus Join(void);

    /**
     * Indicate whether a stop has been requested for this thread.
     *
     * @return true iff thread has been signalled to stop.
     */
    bool IsStopping(void) { return isStopping; }

    /**
     * Get the exit value.  Any memory referenced by this pointer must either
     * have been provided to the thread via its argument or allocated on the
     * heap.  Memory referencing any stack space from the recently exited
     * thread is now invalid.
     *
     * @return  The exit value encoded in a void *.
     */
    ThreadReturn GetExitValue(void)
    {
        return exitValue;
    }

    /**
     * Determine if the thread is currently running. A running thread can be stopped and joined.
     *
     * @return  'true' if the thread is running; 'false' otherwise.
     */
    bool IsRunning(void) { return ((state == STARTED) || (state == RUNNING) || (state == STOPPING)); }

    /**
     * Get the name of the thread.
     *
     * @return  A pointer to a C string of the thread name (may be NULL).
     */
    qcc::String GetName(void) const { return funcName; }

    /**
     * Return underlying thread handle.
     *
     * @return  Thread handle
     */
    ThreadHandle GetHandle(void) { return handle; }

    /**
     * Get a reference to the stop er::Event object for use in er::Event::Wait().
     *
     * @return  Reference to the stop er::Event.
     */
    Event& GetStopEvent(void) { return stopEvent; }

    /**
     * Is this thread allowed to block on the specified resource.
     */
    bool CanBlock(const void* resource) { return resource != noBlockResource; }

    /**
     * Sets the no-block resource for this thread. Only this thread can set this flag.
     *
     * @param  The no-block resource to set. Or NULL to clear the no-block resource.
     */
    void SetNoBlock(const void* resource) { if (GetThread() == this) { noBlockResource = resource; } }

    /**
     * Get the alertCode that was set by the caller to Alert()
     *
     * @return The alertCode specified by the caller to Alert.
     */
    uint32_t GetAlertCode() const { return alertCode; }

  protected:

    Event stopEvent;            ///< Event that indicates a stop request when set.

    /**
     * Invoked by the new thread if Start() returns successfully.
     * The new thead exits when this method returns.
     *
     * The default version of Run() calls the thread function passed into
     * the constructor. Override Run() the thread needs to be able
     * to access Thread (or derrived class) members.
     *
     * @param arg  Argument passed in via Start().
     * @return Exit status for thread.
     */
    virtual ThreadReturn STDCALL Run(void* arg);

  private:

    /**
     * Enumeration of thread states.
     */
    enum {
        INITIAL,  /**< Initial thread state - no underlying OS thread */
        STARTED,  /**< Thread has started */
        RUNNING,  /**< Thread is running the thread function */
        STOPPING, /**< Thread has completed the thread function and is cleaning up */
        DEAD      /**< Underlying OS thread is gone */
    } state;

    bool isStopping;                ///< Thread has received a stop request
    qcc::String funcName;           ///< Function name (used mostly in debug output).
    ThreadFunction function;        ///< Thread entry point or NULL is using Run() as entry point
    ThreadHandle handle;            ///< Thread handle.
    ThreadReturn exitValue;         ///< The returned 'value' from Run.
    void* arg;                      ///< Run thread argument.
    unsigned int threadId;          ///< Thread ID used by windows
    ThreadListener* listener;       ///< Listener notified of thread events (or NULL).
    bool isExternal;                ///< If true, Thread is external (i.e. lifecycle not managed by Thread obj)
    const void* noBlockResource;    ///< No-block resource for this thread
    uint32_t alertCode;             ///< Context passed from alerter to alertee

#ifdef QCC_OS_GROUP_POSIX
    struct JoinContext {
        bool hasBeenJoined;         ///< Indicates that this thread has been joined at least once
        Mutex lock;                 ///< Mutex that protects hasBeenJoined
        int32_t count;              ///< Number of threads that have joined this one
    };
    JoinContext* joinCtx;
    bool threadOwnsJoinCtx;         ///< true if Thread object is responsible for deallocating joinCtx
#endif

    /**
     * C callable thread entry point.
     *
     * @param thread    Pointer to *this.
     * @return  Platform abstracted return type defined in \<os\>/Thread.h.
     */
    static ThreadInternalReturn STDCALL RunInternal(void* thread);

    /**
     * Platform specific wrapper around the signal handler.
     *
     * @param signal   Signal number received
     */
    static void SigHandler(int signal);
};

}

#endif
