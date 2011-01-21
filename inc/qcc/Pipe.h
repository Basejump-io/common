/**
 * @file
 *
 * Sink/Source implementation for storing/retrieving bytes
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

#ifndef _QCC_PIPE_H
#define _QCC_PIPE_H

#include <qcc/platform.h>

#include <qcc/String.h>
#include <qcc/Event.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>

namespace qcc {

/**
 * Pipe provides Sink/Source based storage for bytes.
 * Pushing bytes into the Pipe's Sink will cause the bytes to become
 * available at the Source.
 */
class Pipe : public Stream {
  public:

    /**
     * Construct a Pipe.
     */
    Pipe() : str(), outIdx(0), isWaiting(false) { }

    /**
     * Construct a Pipe from an existing string.
     * @param str   Input string.
     */
    Pipe(const qcc::String str) : str(str), outIdx(0), isWaiting(false) { }

    /** Destructor */
    virtual ~Pipe() { }

    /**
     * Pull bytes from the ByteStream
     * This call will block if there are no bytes available.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Timeout in milliseconds.
     * @return   OI_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    QStatus PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout = Event::WAIT_FOREVER);

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
     * Number of bytes avaiable to Pull
     *
     * @return The number of bytes tha can be pulled.
     */
    size_t AvailBytes() { return str.size() - outIdx; }

  private:
    qcc::String str;    /**< storage for byte stream */
    size_t outIdx;      /**< Index into str for next output char */
    bool isWaiting;     /**< true iff a thread is pending in PullBytes */
    Mutex lock;         /**< Mutex that protects pipe */
    Event event;        /**< Event used to signal availability of more bytes */
};

}  /* namespace */

#endif
