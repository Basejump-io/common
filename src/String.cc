/**
 * @file
 *
 * "STL-like" version of string.
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
#include <qcc/atomic.h>
#include <qcc/String.h>
#include <string.h>
#include <new>

#ifdef WIN32
/*
 * memmem not provided on Windows
 */
static const void* memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen)
{
    if (!haystack || !needle) {
        return haystack;
    } else {
        const char* h = (const char*)haystack;
        const char* n = (const char*)needle;
        size_t l = needlelen;
        const char* r = h;
        while (l && (l <= haystacklen)) {
            if (*n++ != *h++) {
                r = h;
                n = (const char*)needle;
                l = needlelen;
            } else {
                --l;
            }
            --haystacklen;
        }
        return l ? NULL : r;
    }
}
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

namespace qcc {

/* Global Data */
char String::emptyString[] = "";


String::String()
{
    context = NULL;
}

String::String(char c, size_t sizeHint)
{
    NewContext(&c, 1, sizeHint);
}

String::String(const char* str, size_t strLen, size_t sizeHint)
{
    if ((str && *str) || sizeHint) {
        NewContext(str, strLen, sizeHint);
    } else {
        context = NULL;
    }
}

String::String(size_t n, char c, size_t sizeHint)
{
    NewContext(NULL, 0, MAX(n, sizeHint));
    ::memset(context->c_str, c, n);
    context->offset += n;
    context->c_str[context->offset] = '\0';
}

String::String(const String& copyMe)
{
    context = copyMe.context;
    IncRef();
}

String::~String()
{
    DecRef(context);
}

String& String::operator=(const String& assignFromMe)
{
    /* avoid unnecessary free/malloc when assigning to self */
    if (&assignFromMe != this) {
        /* Decrement ref of current context */
        DecRef(context);

        /* Reassign this Managed Obj */
        context = assignFromMe.context;

        /* Increment the ref */
        IncRef();
    }

    return *this;
}

char& String::operator[](size_t pos)
{
    if (context && (1 != context->refCount)) {
        NewContext(context->c_str, size(), context->capacity);
    }
    return context ? context->c_str[pos] : *emptyString;
}

bool String::operator<(const String& str) const
{
    /* Include the null in the compare to catch case when two strings have different lengths */
    if (context && str.context) {
        return (context != str.context) && (0 > ::memcmp(context->c_str,
                                                         str.context->c_str,
                                                         MIN(context->offset, str.context->offset) + 1));
    } else {
        return size() < str.size();
    }
}

int String::compare(size_t pos, size_t n, const String& s) const
{
    int ret;
    if (context && s.context) {
        if ((pos == 0) && (context == s.context)) {
            ret = 0;
        } else {
            size_t subStrLen = MIN(context->offset - pos, n);
            size_t sLen = s.context->offset;
            ret = ::memcmp(context->c_str + pos, s.context->c_str, MIN(subStrLen, sLen));
            if ((0 == ret) && (subStrLen < sLen)) {
                ret = -1;
            } else if ((0 == ret) && (subStrLen > sLen)) {
                ret = 1;
            }
        }
    } else {
        if (context && (0 < n) && (npos != pos)) {
            ret = 1;
        } else if (0 < s.size()) {
            ret = -1;
        } else {
            ret = 0;
        }
    }
    return ret;
}

int String::compare(size_t pos, size_t n, const String& s, size_t sPos, size_t sn) const
{
    int ret;
    if (context && s.context) {
        if ((pos == sPos) && (context == s.context)) {
            ret = 0;
        } else {
            size_t subStrLen = MIN(context->offset - pos, n);
            size_t sSubStrLen = MIN(s.context->offset - sPos, sn);
            ret = ::memcmp(context->c_str + pos, s.context->c_str + sPos, MIN(subStrLen, sSubStrLen));
            if ((0 == ret) && (subStrLen < sSubStrLen)) {
                ret = -1;
            } else if ((0 == ret) && (subStrLen > sSubStrLen)) {
                ret = 1;
            }
        }
    } else {
        if (context && (0 < n) && (npos != pos) && (size() > pos)) {
            ret = 1;
        } else if (s.context && (0 < sn) && (npos != sPos) && (s.size() > sPos)) {
            ret = -1;
        } else {
            ret = 0;
        }
    }
    return ret;
}

void String::clear(size_t sizeHint)
{
    DecRef(context);
    context = NULL;
}

String& String::append(const char* str, size_t strLen)
{
    if (NULL == str) return *this;
    if (0 == strLen) strLen = ::strlen(str);
    if (0 == strLen) return *this;

    if (NULL == context) {
        NewContext(NULL, 0, strLen + 8);
    }

    size_t totalLen = strLen + context->offset;
    if ((1 != context->refCount) || (totalLen > context->capacity)) {
        /* Append wont fit in existing allocation or modifying a context with other refs */
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, oldContext->offset, totalLen + totalLen / 2);
        DecRef(oldContext);
    }
    ::memcpy(context->c_str + context->offset, str, strLen);
    context->offset += strLen;
    context->c_str[context->offset] = '\0';
    return *this;
}

String& String::erase(size_t pos, size_t n)
{
    /* Trying to erase past the end of the string, do nothing. */
    if (pos >= size())
        return *this;

    if (context) {
        if (context->refCount != 1) {
            /* Need a new context */
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, context->offset, context->capacity);
            DecRef(oldContext);
        }
        n = MIN(n, size() - pos);
        ::memmove(context->c_str + pos, context->c_str + pos + n, size() - pos - n + 1);
        context->offset -= n;
    }
    return *this;
}

void String::resize(size_t n, char c)
{
    if ((0 < n) && (NULL == context)) {
        NewContext(NULL, 0, n);
    }

    size_t curSize = size();
    if (n < curSize) {
        if (context->refCount == 1) {
            context->offset = n;
            context->c_str[n] = '\0';
        } else {
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, n, n);
            DecRef(oldContext);
        }
    } else if (n > curSize) {
        if (n >= context->capacity) {
            ManagedCtx* oldContext = context;
            NewContext(context->c_str, curSize, n);
            DecRef(oldContext);
        }
        ::memset(context->c_str + curSize, c, n - curSize);
        context->offset = n;
        context->c_str[n] = '\0';
    }
}

void String::reserve(size_t newCapacity)
{
    if ((0 < newCapacity) && (NULL == context)) {
        NewContext(NULL, 0, newCapacity);
    }

    newCapacity = MAX(newCapacity, size());
    if (newCapacity != context->capacity) {
        size_t sz = size();
        ManagedCtx* oldContext = context;
        NewContext(context->c_str, sz, newCapacity);
        DecRef(oldContext);
    }
}

String& String::insert(size_t pos, const char* str, size_t strLen)
{
    if (NULL == str) return *this;
    if (0 == strLen) strLen = ::strlen(str);

    if (NULL == context) {
        NewContext(NULL, 0, strLen);
    }

    pos = MIN(pos, context->offset);

    size_t totalLen = strLen + context->offset;
    if ((1 != context->refCount) || (totalLen > context->capacity)) {
        /* insert wont fit in existing allocation or modifying a context with other refs */
        ManagedCtx* oldContext = context;
        NewContext(oldContext->c_str, oldContext->offset, totalLen + totalLen / 2);
        DecRef(oldContext);
    }
    ::memmove(context->c_str + pos + strLen, context->c_str + pos, context->offset - pos + 1);
    ::memcpy(context->c_str + pos, str, strLen);
    context->offset += strLen;
    return *this;
}

size_t String::find(const char* str, size_t pos) const
{
    if (NULL == context) return npos;

    const char* base = context->c_str;
    const char* p = static_cast<const char*>(::memmem(base + pos, context->offset - pos, str, ::strlen(str)));
    return p ? p - base : npos;
}

size_t String::find(const String& str, size_t pos) const
{
    if (NULL == context) return npos;
    if (0 == str.size()) return 0;

    const char* base = context->c_str;
    const char* p = static_cast<const char*>(::memmem(base + pos,
                                                      context->offset - pos,
                                                      str.context->c_str,
                                                      str.context->offset));
    return p ? p - base : npos;
}

size_t String::find_first_of(const char c, size_t pos) const
{
    if (NULL == context) return npos;

    const char* ret = ::strchr(context->c_str + pos, (int) c);
    return ret ? (ret - context->c_str) : npos;
}

size_t String::find_last_of(const char c, size_t pos) const
{
    if (NULL == context) return npos;

    size_t i = MIN(pos, size());
    while (i-- > 0) {
        if (c == context->c_str[i]) {
            return i;
        }
    }
    return npos;
}

size_t String::find_first_of(const char* set, size_t pos) const
{
    if (NULL == context) return npos;

    size_t i = pos;
    size_t endIdx = size();
    while (i < endIdx) {
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                return i;
            }
        }
        ++i;
    }
    return npos;
}

size_t String::find_first_not_of(const char* set, size_t pos) const
{
    if (NULL == context) return npos;

    size_t i = pos;
    size_t endIdx = size();
    while (i < endIdx) {
        bool found = false;
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return i;
        }
        ++i;
    }
    return npos;
}

size_t String::find_last_not_of(const char* set, size_t pos) const
{
    if (NULL == context) return npos;

    size_t i = MIN(pos, size());
    while (i-- > 0) {
        bool found = false;
        for (const char* c = set; *c != '\0'; ++c) {
            if (*c == context->c_str[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    return npos;
}

String String::substr(size_t pos, size_t n) const
{
    if (pos <= size()) {
        return String(context->c_str + pos, MIN(n, size() - pos));
    } else {
        return String();
    }
}

bool String::operator==(const String& other) const
{
    if (context == other.context) {
        /* If both contexts point to the same memory then they are, by definition, the same. */
        return true;
    }
    size_t otherSize = other.size();
    if (context && (0 < otherSize)) {
        if (context->offset != other.context->offset) {
            return false;
        }
        return (0 == ::memcmp(context->c_str, other.context->c_str, context->offset));
    } else {
        /* Both strings must be empty or they aren't equal */
        return (size() == other.size());
    }
}

void String::NewContext(const char* str, size_t strLen, size_t sizeHint)
{
    if (NULL == str) {
        strLen = 0;
    } else if (0 == strLen) {
        strLen = ::strlen(str);
    }
    size_t capacity = MAX(MinCapacity, MAX(strLen, sizeHint));
    size_t mallocSz = capacity + 1 + sizeof(ManagedCtx) - MinCapacity;
    context = new (malloc(mallocSz))ManagedCtx();
    context->refCount = 1;

    context->capacity = static_cast<uint32_t>(capacity);
    context->offset = static_cast<uint32_t>(strLen);
    if (str) {
        ::memcpy(context->c_str, str, strLen);
    }
    context->c_str[strLen] = '\0';
}

void String::IncRef()
{
    /* Increment the ref count */
    if (context) {
        IncrementAndFetch(&context->refCount);
    }
}

void String::DecRef(ManagedCtx* ctx)
{
    /* Decrement the ref count */
    if (ctx) {
        uint32_t refs = DecrementAndFetch(&ctx->refCount);
        if (0 == refs) {
            ctx->ManagedCtx::~ManagedCtx();
            free(ctx);
        }
    }
}

}

/* Global function (not part of qcc namespace) */
qcc::String operator+(const qcc::String& s1, const qcc::String& s2)
{
    qcc::String ret(s1);
    return ret.append(s2);
}

