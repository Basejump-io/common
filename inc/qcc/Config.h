/**
 * @file Config.h
 *
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
#ifndef _QCC_CONFIG_H
#define _QCC_CONFIG_H

#include <qcc/platform.h>
#include <map>

#include <qcc/Mutex.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>

/** @internal */
#define QCC_MODULE "CONFIG"

namespace qcc {

/**
 * Config implements a file-based, key/value store.
 * The file format is a simplified version of Microsoft's INI file format.
 */
class Config {
  public:

    /**
     * Return a pointer to the singleton.  Creates the singleton if not already instantiated.
     *
     * @return  Pointer to the config variable singleton.
     */
    static Config* GetConfig(void)
    {
        static Mutex singletonLock;
        static Config* config = NULL;                                  ///< Config variable singleton.

        singletonLock.Lock();
        if (config == NULL) {
            config = new Config();
        }
        singletonLock.Unlock();

        return config;
    }

  private:
    std::map<qcc::String, qcc::String> nameValuePairs;     /**< Key/value pairs from config file */

    /**
     * Default constructor is private to ensure singleton useage.
     */
    Config(void);


  public:

    /**
     * Get value for key.
     *
     * @param key   The key.
     * @param defaultValue   Default value used if key is not found in config file.
     * @return Value associated with key or defaultValue if no such entry exists.
     */
    qcc::String GetValue(const char* key, const char* defaultValue = NULL) const {
        qcc::String value = (defaultValue ? defaultValue : "");
        std::map<qcc::String, qcc::String>::const_iterator it;

        it = nameValuePairs.find(key);
        if (nameValuePairs.end() != it) {
            value = it->second;
        }

        return value;
    }

    /**
     * Get the value for a key as an unsigned number.
     *
     * @param key             The key.
     * @param defaultValue    Default value used if key is not found (or is unparsable) in config file.
     * @return  Value associated with key or default value if not found.
     */
    uint32_t GetValueNumeric(qcc::String key, uint32_t defaultValue = 0) const {
        uint32_t valNumeric = defaultValue;
        std::map<qcc::String, qcc::String>::const_iterator it;

        it = nameValuePairs.find(key);
        if (nameValuePairs.end() != it) {
            valNumeric = StringToU32(it->second, 10, defaultValue);
        }

        return valNumeric;
    }
};

}

#undef QCC_MODULE
#endif
