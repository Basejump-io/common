#ifndef _QCC_PLATFORM_H
#define _QCC_PLATFORM_H
/**
 * @file
 *
 * This file just wraps including actual OS and toolchain specific header
 * files depding on the OS group setting.
 */

/******************************************************************************
 * Copyright 2010-2011, Qualcomm Innovation Center, Inc.
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

#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/platform_types.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/platform_types.h>
#include <qcc/windows/mapping.h>
#elif defined(QCC_OS_GROUP_WINRT)
#include <qcc/winrt/platform_types.h>
#include <qcc/winrt/mapping.h>
#else
#error No OS GROUP defined.
#endif

#if defined(__GNUC__)


#define GCC_VERSION ((__GNUC__ * 10000) + (__GNUC_MINOR__ * 100) + __GNUC_PATCHLEVEL__)
#if (GCC_VERSION < 40700L)
/*
 * Versions of GCC prior to 4.7.0 have an annoying but intentional bug where
 * __cplusplus is set to 1 rather than the appropriate date code so that it
 * would be compatible with Solaris 8.
 */

#if (GCC_VERSION >= 40600L) && defined(__GXX_EXPERIMENTAL_CXX0X__)
/*
 * GCC 4.6.x supports C++11, at least in terms of std::tr1::unordered_map, etc. when the
 * -std=gnu++0x option is passed in.  Thus, fix the value of __cplusplus.
 */
#undef __cplusplus
#define __cplusplus 201100L
#endif  // GCC version >= 4.6 and -std=gnu++0x
#endif  // GCC version < 4.7



#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define QCC_DEPRECATED(func) func __attribute__((deprecated)) /**< mark a function as deprecated in gcc. */
#else
#define QCC_DEPRECATED(func) func /**< not all gcc versions support the deprecated attribute. */
#endif

#define QCC_DLLEXPORT


#elif defined(_MSC_VER)

#define QCC_DEPRECATED(func) __declspec(deprecated) func /**< mark a function as deprecated in msvc. */

#define QCC_DLLEXPORT __declspec(dllexport)


#else /* Some unknown compiler */

#define QCC_DEPRECATED(func); /**< mark a function as deprecated. */

#endif /* Compiler type */

/** Boolean type for C */
typedef int32_t QCC_BOOL;
/** Boolean logic true for QCC_BOOL type*/
#define QCC_TRUE 1
/** Boolean logic false for QCC_BOOL type*/
#define QCC_FALSE 0

#endif // _QCC_PLATFORM_H
