/**
 * @file STLContainer.h
 *
 *
 *
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
 *
 ******************************************************************************/

#ifndef _STLCONTAINER_H
#define _STLCONTAINER_H

#include <qcc/platform.h>


/***************************
 * STANDARD C++11
 ***************************/
#if (__cplusplus >= 201100L)
/*
 * The compiler is conformant to the C++11 standard.  Use std::tr1::unordered_map,
 * etc. directly.
 */
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }


#else
/*
 * Compiling with a C++ compiler that predates the C++11 standard.  Need to
 * map compiler specific arrangements to match the standard arrangement.
 */



/***************************
 * DARWIN
 ***************************/
#if defined(QCC_OS_DARWIN)
/*
 * Darwin (Mac OSX and iOS) currently put std::tr1::unordered_map, etc. under the tr1
 * subdirectory and place their template classes in the std::tr1 namespace.
 */
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std { namespace tr1 {
#define _END_NAMESPACE_CONTAINER_FOR_HASH } }

namespace std {
/*
 * Map everything in the std::tr1 namespace to the std namespace.
 */
using namespace std::tr1;
}



/***************************
 * MSVC 2008, 2010
 ***************************/
#elif defined(_MSC_VER)
#if (_MSC_VER >= 1500)
/*
 * MSVC 2008 and later provide std::tr1::unordered_map, etc. in the standard location.
 */
#include <unordered_map>
#include <unordered_set>

#if (_MSC_VER >= 1600)
/*
 * MSVC 2010 actually follows the C++11 standard for std::tr1::unordered_map, etc. but
 * we are here because it still defined __cplusplus to 199711.
 */
#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }

#elif (_MSC_VER >= 1500)
/*
 * MSVC 2008 puts std::tr1::unordered_map, etc. in the std::tr1 namespace.
 */
#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace std { namespace tr1 {
#define _END_NAMESPACE_CONTAINER_FOR_HASH } }

namespace std {
/*
 * Map everything in the std::tr1 namespace to the std namespace.
 */
using namespace std::tr1;
}

#endif  // MSVC versions

#endif  // MSVC version >= 2008


/***************************
 * GCC
 ***************************/
#elif defined(__GNUC__)
/*
 * Older versions of GCC use hash_map, etc. instead of std::tr1::unordered_map,
 * etc. respectively.  Thus we need to map the class names to unordered_*.
 * Additionally, the hash_* classes are in the __gnu_cxx namespace.
 */
#include <ext/hash_map>
#include <ext/hash_set>

#define _BEGIN_NAMESPACE_CONTAINER_FOR_HASH namespace __gnu_cxx {
#define _END_NAMESPACE_CONTAINER_FOR_HASH }

#define std::tr1::unordered_map hash_map
#define unordered_multimap hash_multimap
#define unordered_set hash_set
#define unordered_multiset hash_multiset

namespace std {
/*
 * Map everything in the __gnu_cxx namespace to the std namespace.
 */
using namespace __gnu_cxx;
}


#else
#error Unsupported Compiler/Platform

#endif  // Platforms for C++ prior to C++11
#endif  // Standard C++11
#endif  // _STLCONTAINER_H
