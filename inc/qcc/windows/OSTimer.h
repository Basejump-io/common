/**
 * @file
 *
 * OSTimer
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
#ifndef _QCC_OSTIMER_H
#define _QCC_OSTIMER_H

namespace qcc {

class Timer;
class _Alarm;

typedef qcc::ManagedObj<_Alarm> Alarm;

class OSAlarm {
  protected:
};

class OSTimer {
  protected:
    OSTimer(qcc::Timer* timer);

    qcc::Timer* _timer;
};

}

#endif //_QCC_OSTIMER_H
