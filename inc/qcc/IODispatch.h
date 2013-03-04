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
#ifndef _QCC_IODISPATCH_H
#define _QCC_IODISPATCH_H

#include <qcc/platform.h>

#include <qcc/Stream.h>
#include <qcc/Timer.h>
#include <Status.h>
#include <map>
namespace qcc {

/* Forward References */
class IODispatch;

/* Different types of callbacks possible */
enum CallbackType {
    IO_INVALID = 0,
    IO_READ,
    IO_WRITE,
    IO_TIMEOUT,
    IO_EXIT
};
enum StoppingState {
    IO_RUNNING = 0,
    IO_STOPPING,
    IO_STOPPED

};
/**
 * An IO Read listener is capable of receiving read and timeout callbacks
 */
class IOReadListener {
  public:
    virtual ~IOReadListener() { };
    virtual QStatus ReadCallback(Source& source) = 0;
    virtual QStatus LinkTimeoutCallback(Source& source) = 0;
};

/**
 * An IO Write listener is capable of receiving write callbacks
 */
class IOWriteListener {
  public:
    virtual ~IOWriteListener() { };
    virtual QStatus WriteCallback(Sink& sink) = 0;
};

/**
 * An IO Exit listener is capable of receiving exit callbacks
 */
class IOExitListener {
  public:
    virtual ~IOExitListener() { };
    virtual void ExitCallback() = 0;

};

/**
 * The context that will be passed into the AlarmTriggered callback
 */
struct CallbackContext {
    Stream* stream;
    CallbackType type;
    CallbackContext() : stream(NULL), type(IO_INVALID) { }
    CallbackContext(Stream* stream, CallbackType type) : stream(stream), type(type) { }
};


struct IODispatchEntry {
    /* Contexts for different callbacks associated with this stream
     */
    CallbackContext* readCtxt;
    CallbackContext* writeCtxt;
    CallbackContext* timeoutCtxt;
    CallbackContext* exitCtxt;

    /* Alarms associated with this stream */
    Alarm readAlarm;
    Alarm linkTimeoutAlarm;
    Alarm writeAlarm;

    /* Listeners for this stream */
    IOReadListener* readListener;
    IOWriteListener* writeListener;
    IOExitListener* exitListener;

    bool readEnable;        /* Whether read is currently enabled for this stream */
    bool writeEnable;       /* Whether write is currently enabled for this stream */
    bool readInProgress;    /* Whether read is currently in progress for this stream */
    bool writeInProgress;   /* Whether write is currently in progress for this stream */
    uint32_t linkTimeout;   /* The link timeout for this stream ( set using RemoteEndpoint::SetLinkTimeout) in seconds */
    StoppingState stopping_state;          /* Whether this stream is in the process of being stopped*/

    /**
     * Default Unusable entry
     *
     */
    IODispatchEntry() : readEnable(false), writeEnable(false), readInProgress(false), writeInProgress(false), stopping_state(IO_RUNNING) { }

    /**
     * Constructor
     *
     * @param stream             The stream that this entry is associated with.
     * @param readListener       The listener to which read and timeout callbacks for this stream must be made.
     * @param writeListener      The listener to which write callbacks for this stream must be made.
     * @param exitListener       The listener to which exit callbacks for this stream must be made.
     * @param linkTimeout        The link timeout for this stream.
     * @param readEnable         Whether to enable read for this stream.
     * @param writeEnable        Whether to enable write for this stream.
     */
    IODispatchEntry(Stream* stream, IOReadListener* readListener, IOWriteListener* writeListener, IOExitListener* exitListener,
                    uint32_t linkTimeout = 0, bool readEnable = true, bool writeEnable = true, bool readInProgress = false, bool writeInProgress = false) :
        readCtxt(new CallbackContext(stream, IO_READ)),
        writeCtxt(new CallbackContext(stream, IO_WRITE)),
        timeoutCtxt(new CallbackContext(stream, IO_TIMEOUT)),
        exitCtxt(new CallbackContext(stream, IO_EXIT)),
        readListener(readListener),
        writeListener(writeListener),
        exitListener(exitListener),
        readEnable(readEnable),
        writeEnable(writeEnable),
        readInProgress(readInProgress),
        writeInProgress(writeInProgress),
        linkTimeout(linkTimeout),
        stopping_state(IO_RUNNING)
    { }
};

class IODispatch : public Thread, public AlarmListener {
  public:
    IODispatch(const char* name, uint32_t concurrency);
    ~IODispatch();

    /**
     * Start the IODispatch and timer.
     *
     * @return  ER_OK if successful.
     */
    QStatus Start();

    /**
     * Stop the IODispatch and associated timer.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop();

    /**
     * Join the IODispatch thread and timer.
     * Block the caller until all the thread and timer are stopped.
     *
     * @return ER_OK if successful.
     */
    QStatus Join();

    /**
     * Start a stream with this IODispatch.
     *
     * @param stream           The stream on which to wait for IO events.
     * @param readListener     The object to call in case of a read event.
     * @param writeListener    The object to call in case of a write event.
     * @param exitListener     The object to call in case of a exit event.
     * @param linkTimeout      The timeout for this link - determines when a timeout event will be fired.
     * @return ER_OK if successful.
     */
    QStatus StartStream(Stream* stream, IOReadListener* readListener, IOWriteListener* writeListener, IOExitListener* exitListener);

    /**
     * Stop a stream previously started with this IODispatch.
     * @param stream           The stream on which to wait for IO events.
     * @return ER_OK if successful.
     */
    QStatus StopStream(Stream* stream);

    /**
     * Stop a stream previously started with this IODispatch.
     * @param stream           The stream on which to wait for IO events.
     * @return ER_OK if successful.
     */
    QStatus JoinStream(Stream* stream);

    /**
     * Enable read callbacks to be triggered for a particular source.
     * @param stream           The stream for which callbacks are to be enabled.
     * @return ER_OK if successful.
     */
    QStatus EnableReadCallback(const Source* source);

    /**
     * Disable read callbacks to be triggered for a particular source.
     * @param stream           The stream for which callbacks are to be disabled.
     * @return ER_OK if successful.
     */
    QStatus DisableReadCallback(const Source* source);

    /**
     * Enable write callbacks to be triggered for a particular sink. This informs
     * the main thread to check when the sink FD is ready to be written to.
     * @param stream           The stream for which callbacks are to be enabled.
     * @return ER_OK if successful.
     */
    QStatus EnableWriteCallback(Sink* sink);

    /**
     * Enable write callbacks to be triggered for a particular sink. This adds
     * a write alarm so that this sink can be written to immediately.
     * @param stream           The stream for which callbacks are to be enabled.
     * @return ER_OK if successful.
     */
    QStatus EnableWriteCallbackNow(Sink* sink);

    /**
     * Disable write callbacks to be triggered for a particular source.
     * @param stream           The stream for which callbacks are to be disabled.
     * @return ER_OK if successful.
     */
    QStatus DisableWriteCallback(const Sink* sink);

    /**
     * Set the link timeout for a particular source.
     * @param stream           The stream for which link timeout needs to be set.
     * @param linkTimeout      Desired link timeout.
     * @return ER_OK if successful.
     */
    QStatus SetLinkTimeout(const Source* source, uint32_t linkTimeout);

    /**
     * Enable link timeout callbacks to be triggered for a particular source.
     * @param stream           The stream for which callbacks are to be enabled.
     * @return ER_OK if successful.
     */

    QStatus EnableTimeoutCallback(const Source* source);

    /**
     * Process a read/write/timeout/exit callback.
     */
    void AlarmTriggered(const Alarm& alarm, QStatus reason);

    /**
     * IODispatch main thread
     */
    virtual ThreadReturn STDCALL Run(void* arg);

  private:
    Timer timer;                                /* The timer used to add and process callbacks */
    Mutex lock;                                 /* Lock for mutual exclusion of dispatchEntries */
    std::map<Stream*, IODispatchEntry> dispatchEntries; /* map holding details of various streams registered with this IODispatch */
    bool reload;                                /* Flag used for synchronization of various methods with the Run thread */
    bool isRunning;                             /* Whether the run thread is still running. */
    int32_t numAlarmsInProgress;                /* Number of alarms currently in progress. */
    int32_t crit;
};



}

#endif

