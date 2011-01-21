/**
 * @file
 *
 * Define atomic read-modify-write memory options
 */

/******************************************************************************
 * $Revision: 14/6 $
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
#ifndef _POSIX_QCC_ATOMIC_H
#define _POSIX_QCC_ATOMIC_H

#include <qcc/platform.h>

namespace qcc {

#ifdef QCC_CPU_ARM

/**
 * Increment an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be incremented.
 * @return  New value (afer increment) of *mem
 */
inline int32_t IncrementAndFetch(volatile int32_t* mem)
{
    int ret;
    unsigned long exclusive;

    __asm__ __volatile__ (
        "1:	ldrex	%0, [%2]\n"
        "	add	%0, %0, #1\n"
        "	strex	%1, %0, [%2]\n"
        "	teq	%1, #0\n"
        "	bne	1b"
        : "=&r" (ret), "=&r" (exclusive)
        : "r" (mem)
        : "cc");

    return ret;
}

/**
 * Decrement an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be decremented.
 * @return  New value (after decrement) of *mem
 */
inline int32_t DecrementAndFetch(volatile int32_t* mem)
{
    int ret;
    unsigned long exclusive;

    __asm__ __volatile__ (
        "1:	ldrex	%0, [%2]\n"
        "	sub	%0, %0, #1\n"
        "	strex	%1, %0, [%2]\n"
        "	teq	%1, #0\n"
        "	bne	1b"
        : "=&r" (ret), "=&r" (exclusive)
        : "r" (mem)
        : "cc");

    return ret;
}

#elif defined (QCC_OS_LINUX)

/**
 * Increment an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be incremented.
 * @return  New value (after increment) of *mem
 */
inline int32_t IncrementAndFetch(int32_t* mem) {
    return __sync_add_and_fetch(mem, 1);
}

/**
 * Decrement an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be decremented.
 * @return  New value (afer decrement) of *mem
 */
inline int32_t DecrementAndFetch(int32_t* mem) {
    return __sync_sub_and_fetch(mem, 1);
}

#else

/**
 * Increment an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be incremented.
 * @return  New value (afer increment) of *mem
 */
int32_t IncrementAndFetch(volatile int32_t* mem);

/**
 * Decrement an int32_t and return it's new value atomically.
 *
 * @param mem   Pointer to int32_t to be decremented.
 * @return  New value (afer decrement) of *mem
 */
int32_t DecrementAndFetch(volatile int32_t* mem);

#endif
}

#endif
