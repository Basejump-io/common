/**
 * @file XmlElement.h
 *
 * Extremely simple XML Parser/Generator.
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

#if !defined(QCC_NO_STLPORT)

#define unordered_map hash_map
#define unordered_multimap hash_multimap
#define unordered_set hash_set

#if defined(__GNUC__) && !defined(ANDROID)
#include <ext/hash_map>
#include <ext/hash_set>
#else
#include <hash_map>
#include <hash_set>
#endif
#else
#include <unordered_map>
#include <unordered_set>
#endif

#endif

