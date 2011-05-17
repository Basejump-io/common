/**
 * @file
 *
 * BufferedSink is an Sink wrapper that attempts to write fixed size blocks
 * to an underyling (wrapped) Sink. It is typically used by Sinks which are
 * slow or otherwise sensitive to small chunk writes.
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

#include <qcc/platform.h>

#include <assert.h>
#include <string.h>

#include <qcc/BufferedSink.h>
#include <qcc/Debug.h>

using namespace std;
using namespace qcc;

#define QCC_MODULE "BUFSTREAM"

BufferedSink::BufferedSink(Sink& sink, size_t minChunk)
    : sink(sink),
    event(sink.GetSinkEvent()),
    minChunk(minChunk),
    buf(new uint8_t[minChunk]),
    wrPtr(buf),
    completeIdx(0),
    isBuffered(false)
{
    QCC_DbgTrace(("BufferedSink(%p, %d)", &sink, minChunk));
}

BufferedSink::~BufferedSink()
{
    QCC_DbgTrace(("~BufferedSink()"));
    delete [] buf;
}

QStatus BufferedSink::PushBytes(const void* dataIn, size_t numBytes, size_t& numSent)
{
    QStatus status = ER_OK;
    const uint8_t* data = static_cast<const uint8_t*>(dataIn);

    QCC_DbgTrace(("BufferedSink::PushBytes(<>, %d, <>)", numBytes));

    /* Pass through if write buffering is disabled */
    if (!isBuffered) {
        return sink.PushBytes(dataIn, numBytes, numSent);
    }

    size_t curBytes = wrPtr - buf;
    if (curBytes + numBytes < minChunk) {
        /* data fits in buf */
        memcpy(wrPtr, data, numBytes);
        wrPtr += numBytes;
        numSent = numBytes;
    } else {
        /* data doesn't fit in buf */
        size_t ns = minChunk;
        if (curBytes > 0) {
            /* buf is not empty. Fill with first part of data */
            memcpy(wrPtr, data, minChunk - curBytes);
            status = sink.PushBytes(buf, minChunk, ns);
            QCC_DbgHLPrintf(("BufferedSink: (1) Pushed %d:%d bytes (%d)", minChunk, ns, status));
            if (status == ER_OK) {
                wrPtr = buf;
                numSent = (ns > curBytes) ? (ns - curBytes) : 0;
                data += numSent;
            }
        }
        /* Copy full size chunks of data if initial chunk fully sent */
        if ((status == ER_OK) && (ns == minChunk)) {
            while ((numBytes - numSent) > minChunk) {
                status = sink.PushBytes(data, minChunk, ns);
                QCC_DbgHLPrintf(("BufferedSink: (2) Pushed %d:%d bytes (%d)", minChunk, ns, status));
                if ((status == ER_OK) && (ns > 0)) {
                    numSent += ns;
                    data += numSent;
                }
                if ((status != ER_OK) || (ns != minChunk)) {
                    break;
                }
            }
            /* Copy final fragment to buf if all previous chunks were sent */
            if ((status == ER_OK) && (ns == minChunk) && ((numBytes - numSent) > 0)) {
                assert(numBytes - numSent < minChunk);
                memcpy(buf, data, numBytes - numSent);
                wrPtr = buf + numBytes - numSent;
                numSent = numBytes;
            }
        }
    }
    return status;
}

QStatus BufferedSink::Flush()
{
    QCC_DbgTrace(("BufferedSink::Flush()"));

    QStatus status = ER_OK;
    if (wrPtr > buf + completeIdx) {
        size_t sb;
        status = sink.PushBytes(buf + completeIdx, wrPtr - buf - completeIdx, sb);
        QCC_DbgHLPrintf(("BufferedSink: (3) Pushed %d:%d bytes (%d)", wrPtr - buf - completeIdx, sb, status));
        if (status == ER_OK) {
            if (sb == (wrPtr - buf - completeIdx)) {
                wrPtr = buf;
                completeIdx = 0;
            } else {
                status = ER_WOULDBLOCK;
                completeIdx = sb;
            }
        }
    }
    return status;
}
