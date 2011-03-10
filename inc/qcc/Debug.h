/**
 * @file
 *
 * Define some convenience macros for debugging.
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
#ifndef _QCC_DEBUG_H
#define _QCC_DEBUG_H

#include <qcc/platform.h>

#include <stdio.h>

/** @internal DEBUG */
#define QCC_MODULE "DEBUG"

/**
 * Macro for printing out error messagages.
 *
 * @param _status   Status code related to the message.
 * @param _msg      Parenthesized arguments to be passed to printf
 *                  [e.g. ("Disconnecting from %s", nodename)].
 *                  String should not end with a new line.
 */
#define QCC_LogError(_status, _msg)                                     \
    do {                                                                \
        void* ctx = _QCC_DbgPrintContext _msg;                          \
        _QCC_DbgPrintAppend(ctx, ": %s", QCC_StatusText(_status));      \
        _QCC_DbgPrintProcess(ctx, DBG_LOCAL_ERROR, QCC_MODULE, __FILE__, __LINE__); \
    } while (0)

/**
 * Macro for high level debug prints.  This is intended for high level summary
 * debug information.  Use QCC_DbgPrintf for more detailed debug information.
 *
 * @param _msg      "printf" parameters in parentheses.
 *                  Example: ("value: %d", variable)
 */
#define QCC_DbgHLPrintf(_msg)      _QCC_DbgPrint(DBG_HIGH_LEVEL, _msg)

/**
 * Macro for general purpose debug prints.
 *
 * @param _msg      "printf" parameters in parentheses.
 *                  Example: ("value: %d", variable)
 */
#define QCC_DbgPrintf(_msg)      _QCC_DbgPrint(DBG_GEN_MESSAGE, _msg)

/**
 * Macro for tracing the entrace to functions.  This macro should be called
 * first thing in a function.
 *
 * @param _msg      "printf" parameters in parentheses.
 *                  Example: ("value: %d", variable)
 */
#define QCC_DbgTrace(_msg)    _QCC_DbgPrint(DBG_API_TRACE, _msg)

/**
 * Macro for dumping data received from remote connections.
 *
 * @param _data Pointer to the data to be dumped.
 * @param _len  Length of the data to be dumped.
 */
#define QCC_DbgRemoteData(_data, _len)  _QCC_DbgDumpData(DBG_REMOTE_DATA, _data, _len)

/**
 * Macro for dumping local data that will be sent to remote connections.
 *
 * @param _data Pointer to the data to be dumped.
 * @param _len  Length of the data to be dumped.
 */
#define QCC_DbgLocalData(_data, _len)   _QCC_DbgDumpData(DBG_LOCAL_DATA, _data, _len)

/**
 * Macro for reporting errors in received data from remote connections.
 *
 * @param _msg      "printf" parameters in parentheses.
 *                  Example: ("value: %d", variable)
 */
#define QCC_DbgRemoteError(_msg) _QCC_DbgPrint(DBG_REMOTE_ERROR, _msg)


/**
 * Macro to make conditional compilation of simple lines of code dependent on
 * debug vs. release easier to read (and thus maintain).  This should be used
 * for single statements only.  (Example: QCC_DEBUG_ONLY(printf("only print in
 * debug mode\n"));)
 *
 * @param _cmd  Single, simple, complete C statement to be compiled out in release mode.
 */
#if defined(NDEBUG)
#define QCC_DEBUG_ONLY(_cmd) do { } while (0)
#else
#define QCC_DEBUG_ONLY(_cmd) do { _cmd; } while (0)
#endif


/**
 * @internal
 * Generalized macro for printing debug messages for code built in debug mode.
 *
 * @param _msgType  Debug message mode defined in DbgMode enum.
 * @param _msg      "printf" parameters in parentheses.
 *                  Example: ("value: %d", variable)
 */
#if defined(NDEBUG)
#define _QCC_DbgPrint(_msgType, _msg) do { } while (0)
#else
#define _QCC_DbgPrint(_msgType, _msg)                                   \
    do {                                                                \
        if (_QCC_DbgPrintCheck((_msgType), QCC_MODULE)) {               \
            void* _ctx = _QCC_DbgPrintContext _msg;                    \
            _QCC_DbgPrintProcess(_ctx, (_msgType), QCC_MODULE, __FILE__, __LINE__); \
        }                                                               \
    } while (0)
#endif


/**
 * @internal
 * Generalized macro for dumping arrays of data.
 *
 * @param _msgType  Debug message mode defined in DbgMode enum.
 * @param _data     Pointer to data to dump.
 * @param _len      Length of data to dump.
 */
#if defined(NDEBUG)
#define _QCC_DbgDumpData(_msgType, _data, _len) do { } while (0)
#else
#define _QCC_DbgDumpData(_msgType, _data, _len)                         \
    _QCC_DbgDumpHex((_msgType), QCC_MODULE, __FILE__, __LINE__, # _data, (_data), (_len))
#endif



#ifdef __cplusplus
extern "C" {
#endif


/**
 * This function may be used by multi-threaded apps to print to STDOUT in a
 * manner that prevents the output from one print statement from becomming
 * interspersed with another print.
 *
 * @param fmt   Format string.
 * @param ...   Arguments for filling out format string.
 *
 * @return  Number of characters printed.  Negative value indicates an error.
 */
int QCC_SyncPrintf(const char* fmt, ...);


/**
 * List of debug modes.
 */
typedef enum {
    DBG_LOCAL_ERROR,    /**< Error messages locally generated.  (Should only
                         *   be used by QCC_LogError().) */
    DBG_REMOTE_ERROR,   /**< Problem detected with data from remote host. */
    DBG_HIGH_LEVEL,     /**< High level debug information. */
    DBG_GEN_MESSAGE,    /**< General debug message. */
    DBG_API_TRACE,      /**< API trace. */
    DBG_REMOTE_DATA,    /**< Communicated data from remote host. */
    DBG_LOCAL_DATA      /**< Local data. */
} DbgMsgType;


/**
 * Initialize the debug control.
 * Calling this init method causes the debug control to re-read its environment
 * settings and re-initialize accordingly.
 */
void QCC_InitializeDebugControl(void);

/**
 * Prototype for the debug message callback.  This callback allows application
 * code to recieve debug message rather than the debug messages going to
 * STDERR or a file.
 *
 * @param type      Type of debug message.  One of the DbgMsgType enumerations.
 * @param module    A C string identifying the module that the debug message is from.
 * @param msg       A C string containing the debug message.
 * @param context   An opaque pointer to context data provided by the application
 *                  when registering the callback.
 */
typedef void (*QCC_DbgMsgCallback)(DbgMsgType type,
                                   const char* module,
                                   const char* msg,
                                   void* context);


/**
 * Allows the application to define it's own debug and error message handler.
 * This is intended for more generic handling of debug messages than what
 * QCC_RegisterOutputFile() can provide.
 *
 * @param cb        Callback to a function for handling debug messages.
 * @param context   Pointer to application data.
 */
void QCC_RegisterOutputCallback(QCC_DbgMsgCallback cb, void* context);

/**
 * Set the FILE stream where debug and error output should go.  If not called then
 * stderr is used as a default.  This should only be called once at start up.
 * See QCC_RegisterOutputCallback() for an alternate that is more generic.
 *
 * @param file  Opened FILE pointer.
 */
void QCC_RegisterOutputFile(FILE* file);


/**
 * @internal
 * Creates and prepares a debug context for printing debug output.
 *
 * @param fmt  A printf() style format specification.
 */
void* _QCC_DbgPrintContext(const char* fmt, ...);

/**
 * @internal
 * Appends the formatted string to the debug context.
 *
 * @param ctx  Debug context created by _QCC_DbgPrintContext.
 * @param fmt  A printf() style format specification.
 */
void _QCC_DbgPrintAppend(void* ctx, const char* fmt, ...);

/**
 * @internal
 * Prints the debug output.
 *
 * @param ctx  Debug context created by _QCC_DbgPrintContext.
 * @param type      The debug type.
 * @param module    The module name.
 * @param filename  Filename where the debug message is.
 * @param lineno    Line number where the debug message is.
 */
void _QCC_DbgPrintProcess(void* ctx, DbgMsgType type, const char* module, const char* filename, int lineno);


/**
 * @internal
 * Check if the module is set for printing debug of the specifed type.
 *
 * @param type      The debug type.
 * @param module    The module name.
 *
 * @return  1 = print, 0 = don't print
 */
int _QCC_DbgPrintCheck(DbgMsgType type, const char* module);


/**
 * @internal
 * Dumps data to the debug output.
 *
 * @param type      The debug type.
 * @param module    The module name.
 * @param filename  Filename where the debug message is.
 * @param lineno    Line number where the debug message is.
 * @param dataStr   String representation of the data pointer 'name' to be dumped.
 * @param data      Pointer to the data to be dumped.
 * @param dataLen   Length of the data to be dumped.
 */
void _QCC_DbgDumpHex(DbgMsgType type, const char* module, const char* filename, int lineno,
                     const char* dataStr, const void* data, size_t dataLen);


#ifdef __cplusplus
}
#endif

#undef QCC_MODULE

#endif

