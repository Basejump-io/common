/**
 * @file
 *
 * OS specific utility functions
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

#include <qcc/platform.h>

#include <string.h>
#include <windows.h>
#include <process.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <security.h>
#include <secext.h>

#include <qcc/Util.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>



#define QCC_MODULE  "UTIL"


uint32_t qcc::GetPid()
{
    return static_cast<uint32_t>(_getpid());
}

static uint32_t ComputeId(const char* buf, size_t len)
{
    QCC_DbgPrintf(("ComputeId %s", buf));
    uint32_t digest[qcc::Crypto_SHA1::DIGEST_SIZE / sizeof(uint32_t)];
    qcc::Crypto_SHA1 sha1;
    sha1.Init();
    sha1.Update((const uint8_t*)buf, (size_t)len);
    sha1.GetDigest((uint8_t*)digest);
    return digest[0];
}

uint32_t qcc::GetUid()
{
    /*
     * Windows has no equivalent of getuid so fake one by creating a hash of the user name.
     */
    char buf[UNLEN];
    ULONG len = UNLEN;
    if (GetUserNameExA(NameUniqueId, buf, &len)) {
        return ComputeId((char*)buf, len);
    } else {
        return ComputeId("nobody", sizeof("nobody") - 1);
    }
}

uint32_t qcc::GetGid()
{
    /*
     * Windows has no equivalent of getgid so fake one by creating a hash of the user's domain name.
     */
    char buf[UNLEN];
    ULONG len = UNLEN;
    if (GetUserNameExA(NameDnsDomain, buf, &len)) {
        qcc::String gp((char*)buf, len);
        size_t pos = gp.find_first_of('\\');
        if (pos != qcc::String::npos) {
            gp.erase(pos);
        }
        return ComputeId((const char*)gp.c_str(), gp.size());
    } else {
        return ComputeId("nogroup", sizeof("nogroup") - 1);
    }
}

uint32_t qcc::GetUsersUid(const char* name)
{
    return ComputeId(name, strlen(name));
}

uint32_t qcc::GetUsersGid(const char* name)
{
    return ComputeId(name, strlen(name));
}

qcc::String qcc::GetHomeDir()
{
    return Environ::GetAppEnviron()->Find("USERPROFILE");
}

qcc::OSType qcc::GetSystemOSType(void)
{
    return qcc::WINDOWS_OS;
}

QStatus qcc::GetDirListing(const char* path, DirListing& listing)
{
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::Exec(const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    return ER_NOT_IMPLEMENTED;
}


QStatus qcc::ExecAs(const char* user, const char* exec, const ExecArgs& args, const qcc::Environ& envs)
{
    return ER_NOT_IMPLEMENTED;
}

class ResolverThread : public qcc::Thread, public qcc::ThreadListener {
  public:
    ResolverThread(qcc::String& hostname, uint8_t*addr, size_t* addrLen);
    virtual ~ResolverThread() { }
    QStatus Get(uint32_t timeoutMs);

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg);
    void ThreadExit(Thread* thread);

  private:
    qcc::String hostname;
    uint8_t*addr;
    size_t* addrLen;
    QStatus status;
    qcc::Mutex lock;
    qcc::Event complete;
    bool threadHasExited;
};

ResolverThread::ResolverThread(qcc::String& hostname, uint8_t* addr, size_t* addrLen)
    : hostname(hostname), addr(addr), addrLen(addrLen), threadHasExited(false)
{
    status = Start(NULL, this);
    if (ER_OK != status) {
        addr = NULL;
        addrLen = NULL;
    }
}

QStatus ResolverThread::Get(uint32_t timeoutMs)
{
    if (addr && addrLen) {
        status = qcc::Event::Wait(complete, timeoutMs);
        if (ER_OK == status) {
            Join();
            status = static_cast<QStatus>(reinterpret_cast<uintptr_t>(GetExitValue()));
        }
    }

    lock.Lock(MUTEX_CONTEXT);
    addr = NULL;
    addrLen = NULL;
    bool deleteThis = threadHasExited;
    QStatus sts = status;
    lock.Unlock(MUTEX_CONTEXT);

    if (deleteThis) {
        Join();
        delete this;
    }
    return sts;
}

void ResolverThread::ThreadExit(Thread*)
{
    lock.Lock(MUTEX_CONTEXT);
    threadHasExited = true;
    bool deleteThis = !addr && !addrLen;
    lock.Unlock(MUTEX_CONTEXT);

    if (deleteThis) {
        Join();
        delete this;
    }
}

void* ResolverThread::Run(void*)
{
    QStatus status = ER_OK;

    struct addrinfo* info = NULL;
    int ret = getaddrinfo(hostname.c_str(), NULL, NULL, &info);
    if (0 == ret) {
        lock.Lock(MUTEX_CONTEXT);
        if (addr && addrLen) {
            if (info->ai_family == AF_INET6) {
                struct sockaddr_in6* sa = (struct sockaddr_in6*) info->ai_addr;
                memcpy(addr, &sa->sin6_addr, qcc::IPAddress::IPv6_SIZE);
                *addrLen = qcc::IPAddress::IPv6_SIZE;
            } else if (info->ai_family == AF_INET) {
                struct sockaddr_in* sa = (struct sockaddr_in*) info->ai_addr;
                memcpy(&addr[qcc::IPAddress::IPv6_SIZE - qcc::IPAddress::IPv4_SIZE], &sa->sin_addr, qcc::IPAddress::IPv4_SIZE);
                *addrLen = qcc::IPAddress::IPv4_SIZE;
            } else {
                status = ER_FAIL;
            }
        }
        lock.Unlock(MUTEX_CONTEXT);
        freeaddrinfo(info);
    } else {
        status = ER_BAD_HOSTNAME;
        QCC_LogError(status, ("getaddrinfo - %s", gai_strerror(ret)));
    }

    complete.SetEvent();
    return (void*) status;
}

QStatus qcc::ResolveHostName(qcc::String hostname, uint8_t addr[], size_t addrSize, size_t& addrLen, uint32_t timeoutMs)
{
    if (qcc::IPAddress::IPv6_SIZE != addrSize) {
        return ER_BAD_HOSTNAME;
    }
    return (new ResolverThread(hostname, addr, &addrLen))->Get(timeoutMs);
}
