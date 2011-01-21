#ifndef _QCC_UTIL_H
#define _QCC_UTIL_H
/**
 * @file
 *
 * This file provides some useful utility macros and wrappers around system APIs
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/Environ.h>
#include <list>
#include <Status.h>


/*
 * Platform util.h will define QCC_TARGET_ENDIAN to one of these.  This will
 * allow for compile time determination of the target system's endianness.
 */
#define QCC_LITTLE_ENDIAN 1234
#define QCC_BIG_ENDIAN    4321


/*
 * Include platform-specific utility macros
 */
#if defined(QCC_OS_GROUP_POSIX)
#include <qcc/posix/util.h>
#elif defined(QCC_OS_GROUP_WINDOWS)
#include <qcc/windows/util.h>
#else
#error No OS GROUP defined.
#endif


/**
 * Returns the size of a statically allocated array
 */
#define ArraySize(a)  (sizeof(a) / sizeof(a[0]))


/**
 * Number of pad bytes needed to align to a specified byte boundary.
 *
 * @param p  The pointer to align
 * @param b  The byte boundary to align it on (b must be a power of 2)
 */
#define PadBytes(p, b) (((b) - reinterpret_cast<size_t>(p)) & ((b) - 1))

/**
 * Return a pointer aligned (up) to a specified byte boundary
 *
 * @param p  The pointer to align
 * @pram b   The byte boundary to align it on (b must be a power of 2)
 */
#define AlignPtr(p, b) ((p) + PadBytes(p, b))


namespace qcc {

/**
 * Return an 8 bit random number.
 *
 * @return An 8 bit random number
 */
uint8_t Rand8();

/**
 * Return a 16 bit random number.
 *
 * @return A 16 bit random number
 */
uint16_t Rand16();

/**
 * Return a cryptographically strong 32 bit random number
 *
 * @return A 32 bit random number
 */
uint32_t Rand32();

/**
 * Return a cryptographically strong 64 bit random number
 *
 * @return A 64 bit random number
 */
uint64_t Rand64();

/**
 * Return the Process ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit Process ID
 */
uint32_t GetPid();

/**
 * Return the User ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit User ID
 */
uint32_t GetUid();

/**
 * Return the Group ID as an unsigned 32 bit integer.
 *
 * @return The 32 bit User ID
 */
uint32_t GetGid();

/**
 * Return the User ID of the named user as an unsigned 32 bit integer.
 *
 * @param name  Username to lookup.
 */
uint32_t GetUsersUid(const char* name);

/**
 * Return the Group ID of the named user as an unsigned 32 bit integer.
 *
 * @param name  Username to lookup.
 */
uint32_t GetUsersGid(const char* name);

/**
 * Return the home directory of the calling user
 */
qcc::String GetHomeDir();

/**
 * Return a string with random, filename safe characters.
 *
 * @param prefix    [optional] resulting string will start with this.
 * @param len       [optional] number of charaters to generate.
 *
 * @return  The string with random characters.
 */
qcc::String RandomString(const char* prefix = NULL, size_t len = 10);


/**
 * Container type for directory listing results.
 */
typedef std::list<qcc::String> DirListing;

/**
 * Get a list of files and subdirectories in the specified path.
 *
 * @param path      The path from which the listing will be gathered.
 * @param listing   [OUT] Collection of filenames in path.
 *
 * @return  ER_OK if directory listing was gathered without problems.
 */
QStatus GetDirListing(const char* path, DirListing& listing);


/**
 * Container type for arguments to a program to be executed.
 */
typedef std::list<qcc::String> ExecArgs;

/**
 * Execute the specified program in a separate process with the specified
 * arguments.
 *
 * @param exec  Program to be executed.
 * @param args  Arguments to be passed to the program.
 * @param envs  Environment variables defining the envrionment the program will run in.
 *
 * @return  ER_OK if program was launched successfully.
 */
QStatus Exec(const char* exec, const ExecArgs& args, const qcc::Environ& envs);

/**
 * Execute the specified program in a separate process with the specified
 * arguments as a different user.
 *
 * @param user  user ID of the user to run as.
 * @param exec  Program to be executed.
 * @param args  Arguments to be passed to the program.
 * @param envs  Environment variables defining the envrionment the program will run in.
 *
 * @return  ER_OK if program was launched successfully.
 */
QStatus ExecAs(const char* user, const char* exec, const ExecArgs& args, const qcc::Environ& envs);


};
#endif
