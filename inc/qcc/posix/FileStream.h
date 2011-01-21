/**
 * @file
 *
 * Sink/Source wrapper FILE operations
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

#ifndef _OS_QCC_FILESTREAM_H
#define _OS_QCC_FILESTREAM_H

#include <qcc/platform.h>

#include <cstdio>
#include <errno.h>
#include <string>

#include <qcc/String.h>
#include <qcc/Stream.h>
#include <Status.h>

namespace qcc {

/**
 * FileSource is an implementation of Source used for reading from files.
 */
class FileSource : public Source {
  public:

    /**
     * Create an FileSource
     *
     * @param fileName   Name of file to read/write
     */
    FileSource(qcc::String fileName);

    /**
     * Create an FileSource from stdin
     */
    FileSource();

    /** Destructor */
    virtual ~FileSource();

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    Event& GetSourceEvent() { return event; }

    /**
     * Check validity of FILE.
     *
     * @return  true iff stream was successfully initialized.
     */
    bool IsValid() { return 0 <= fd; }

  private:
    int fd;        /**< File descriptor */
    Event event;   /**< I/O event */
    bool ownsFd;   /**< true if sink is responsible for closing fd */
};


/**
 * FileSink is an implementation of Sink used to write to files.
 */
class FileSink : public Sink {

  public:

    /**
     * File creation mode.
     */
    typedef enum {
        PRIVATE = 0,        /**< Private to the calling user */
        WORLD_READABLE = 1, /**< World readable */
        WORLD_WRITABLE = 2, /**< World writable */
    } Mode;

    /**
     * Create an FileSink.
     *
     * @param fileName     Name of file to use as sink.
     */
    FileSink(qcc::String fileName, Mode mode = WORLD_READABLE);

    /**
     * Create an FileSink for stdout
     */
    FileSink();

    /** FileSink Destructor */
    virtual ~FileSink();

    /**
     * Push bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

    /**
     * Get the Event that indicates when data can be pushed to sink.
     *
     * @return Event that is signaled when sink can accept more bytes.
     */
    Event& GetSinkEvent() { return event; }

    /**
     * Check validity of FILE.
     *
     * @return  true iff stream was successfully initialized.
     */
    bool IsValid() { return 0 <= fd; }

  private:

    int fd;        /**< File descriptor */
    Event event;   /**< I/O event */
    bool ownsFd;   /**< true if sink is responsible for closing fd */
};

}   /* namespace */

#endif
