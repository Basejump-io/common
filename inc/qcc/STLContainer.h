/**
 * @file STLContainer.h
 *
 *
 *
 */

/******************************************************************************
 * Copyright 2010 - 2011, Qualcomm Innovation Center, Inc.
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

#if defined(QCC_OS_DARWIN)
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#else
#include <unordered_map>
#include <unordered_set>
#endif

#if (defined _MSC_VER && _MSC_VER == 1500) || defined(QCC_OS_DARWIN)
/*
 * For MSVC2008 unordered_map, unordered_multimap, unordered_set, and hash
 * are found in the tr1 libraries while in new version of MSVC and in GNU
 * the libraries are all found in std namespace.
 */
using std::tr1::unordered_map;
using std::tr1::unordered_multimap;
using std::tr1::unordered_set;
using std::tr1::hash;
#else
using std::unordered_map;
using std::unordered_multimap;
using std::unordered_set;
using std::hash;
#endif

#endif

