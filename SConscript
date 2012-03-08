# Copyright 2010 - 2011, Qualcomm Innovation Center, Inc.
# 
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
# 

import os
Import('env')
from os.path import basename

#default crypto for most platforms is openssl
env['CRYPTO'] = 'openssl'

# Bullseye code coverage for 'debug' builds.
if env['VARIANT'] == 'debug':
    if(not(env.has_key('BULLSEYE_BIN'))):
        print('BULLSEYE_BIN not specified')
    else:
        env.PrependENVPath('PATH', env.get('BULLSEYE_BIN'))
        if (not(os.environ.has_key('COVFILE'))):
            print('Error: COVFILE environment variable must be set')
            Exit()
        else:
            env.PrependENVPath('COVFILE', os.environ['COVFILE'])

# Platform specifics for common
if env['OS_GROUP'] == 'windows':
    vars = Variables()
    vars.Add(PathVariable('OPENSSL_BASE', 'Base OpenSSL directory (windows only)', os.environ.get('OPENSSL_BASE')))
    vars.Update(env)
    Help(vars.GenerateHelpText(env))
    env.AppendUnique(LIBS = ['setupapi', 'user32', 'winmm', 'ws2_32', 'iphlpapi', 'secur32', 'Advapi32'])
    # Key of presence of OPENSSL_BASE to decide if to use openssl or window CNG crypto
    if '' == env.subst('$OPENSSL_BASE'):
        if env['OS'] == 'winxp':
            # Must specify OPENSSL_BASE for winXP
            print 'Must specify OPENSSL_BASE when building for WindowsXP'
            Exit()
        else:
            env.AppendUnique(LIBS = ['bcrypt', 'ncrypt', 'crypt32'])
            env['CRYPTO'] = 'cng'
            print 'Using CNG crypto libraries'
    else:
        env.Append(CPPPATH = ['$OPENSSL_BASE/include'])
        env.Append(LIBPATH = ['$OPENSSL_BASE/lib'])
        env.AppendUnique(LIBS = ['libeay32', 'ssleay32'])
        print 'Using OPENSSL crypto libraries'
elif env['OS_GROUP'] == 'winrt':
    env['CRYPTO'] = 'winrt'
    print 'Using WINRT crypto libraries'
    env.AppendUnique(CFLAGS=['/D_WINRT_DLL'])
    env.AppendUnique(CXXFLAGS=['/D_WINRT_DLL'])	
elif env['OS'] == 'linux' or env['OS'] == 'openwrt':
    env.AppendUnique(LIBS =['rt', 'stdc++', 'pthread', 'crypto', 'ssl'])
elif env['OS'] == 'darwin':
    env.AppendUnique(LIBS =['stdc++', 'pthread', 'crypto', 'ssl'])
    if env['CPU'] == 'arm' or env['CPU'] == 'armv7' or env['CPU'] == 'armv7s':
        vars = Variables()
        vars.Add(PathVariable('OPENSSL_ROOT', 'Base OpenSSL directory (darwin only)', os.environ.get('OPENSSL_ROOT')))
        vars.Update(env)
        Help(vars.GenerateHelpText(env))
        if '' == env.subst('$OPENSSL_ROOT'):
            # Must specify OPENSSL_ROOT for darwin, arm
            print 'Must specify OPENSSL_ROOT when building for OS=darwin, CPU=arm'
            Exit()
        env.Append(CPPPATH = ['$OPENSSL_ROOT/include'])
        env.Append(LIBPATH = ['$OPENSSL_ROOT/build/' + os.environ.get('CONFIGURATION') + '-' + os.environ.get('PLATFORM_NAME')])
elif env['OS'] == 'android':
    env.AppendUnique(LIBS = ['m', 'c', 'stdc++', 'crypto', 'log', 'gcc', 'ssl'])
    if (env.subst('$ANDROID_NDK_VERSION') == '7' or
        env.subst('$ANDROID_NDK_VERSION') == '8' or
        env.subst('$ANDROID_NDK_VERSION') == '8b'):
        env.AppendUnique(LIBS = ['gnustl_static'])
elif env['OS'] == 'android_donut':
    env.AppendUnique(LIBS = ['m', 'c', 'stdc++', 'crypto', 'log'])
elif env['OS'] == 'maemo':
    pass
else:
    print 'Unrecognized OS in common: ' + env.subst('$OS')
    Exit()



env.AppendUnique(CPPDEFINES = ['QCC_OS_GROUP_%s' % env['OS_GROUP'].upper()])

# Variant settings
env.VariantDir('$OBJDIR', 'src', duplicate = 0)
env.VariantDir('$OBJDIR/os', 'os/${OS_GROUP}', duplicate = 0)
env.VariantDir('$OBJDIR/crypto', 'crypto/${CRYPTO}', duplicate = 0)

# Setup dependent include directories
hdrs = { 'qcc': env.File(['inc/qcc/Log.h',
                          'inc/qcc/ManagedObj.h',
                          'inc/qcc/String.h',
                          'inc/qcc/atomic.h',
                          'inc/qcc/SocketWrapper.h',
                          'inc/qcc/platform.h']),
         'qcc/${OS_GROUP}': env.File(['inc/qcc/${OS_GROUP}/atomic.h',
                                      'inc/qcc/${OS_GROUP}/platform_types.h',
                                      'inc/qcc/${OS_GROUP}/unicode.h']) }

if env['OS_GROUP'] == 'windows' or env['OS_GROUP'] == 'win8':
    hdrs['qcc/${OS_GROUP}'] += env.File(['inc/qcc/${OS_GROUP}/mapping.h'])

env.Append(CPPPATH = [env.Dir('inc')])

# Build the sources
status_cpp0x_src = ['Status_CPP0x.cc', 'StatusComment.cc']
status_src = ['Status.cc']

srcs = env.Glob('$OBJDIR/*.cc') + env.Glob('$OBJDIR/os/*.cc') + env.Glob('$OBJDIR/crypto/*.cc')

if env['OS_GROUP'] == 'winrt':
    srcs = [ f for f in srcs if basename(str(f)) not in status_cpp0x_src ]

# Make sure Status never gets included from common for contained projects
srcs = [ f for f in srcs if basename(str(f)) not in status_src ]
	
objs = env.Object(srcs)

# Build unit Tests
env.SConscript('unit_test/SConscript', variant_dir='$OBJDIR/unittest', duplicate=0)

ret = (hdrs, objs)

Return('ret')
