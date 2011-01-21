/**
 * @file
 * this file helps handle differences in wchar_t on different OSs
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
#ifndef _PLATFORM_UNICODE_H
#define _PLATFORM_UNICODE_H

#if __SIZEOF_WCHAR_T__ == 4
// GCC normally defines a 4 byte wchar_t
#define ConvertUTF8ToWChar ConvertUTF8toUTF32
#define ConvertWCharToUTF8 ConvertUTF32toUTF8
#define WideUTF UTF32
#else
// GCC will define a 2 byte wchar_t when running under windows or if given the
// -fshort-wchar option.
#define ConvertUTF8ToWChar ConvertUTF8toUTF16          /**< @internal */
#define ConvertWCharToUTF8 ConvertUTF16toUTF8          /**< @internal */
#define WideUTF UTF16                                  /**< @internal */
#endif

#endif
