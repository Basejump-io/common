#ifndef _QCC_TYPES_H
#define _QCC_TYPES_H
/**
 * @file
 *
 * This file defines basic types.
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

#include <stdint.h>
#include <stddef.h>

/* Weird things about bionic when gcc version is < 4.4.0 */
#if 0
#ifdef QCC_OS_ANDROID
#include <sys/types.h>
typedef long ptrdiff_t;
#endif
#endif

#include <unistd.h>

#endif
