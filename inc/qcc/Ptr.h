/**
 * @file
 *
 * This template class provides an intrusive smart pointer implementation.
 */

/******************************************************************************
 * Copyright 2012, Qualcomm Innovation Center, Inc.
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
#ifndef _QCC_PTR_H
#define _QCC_PTR_H

#include <qcc/atomic.h>

namespace qcc {

/**
 * An intrusive smart pointer class.
 */
template <typename T>
class Ptr {
  public:

    /**
     * Initialize a smart pointer to point to NULL.
     */
    Ptr();

    /**
     * Initialize a smart pointer to point to a previously allocated object.
     * The object must inherit from RefCountBase to provide the required
     * reference counting functions.
     */
    Ptr(T* p);

    /**
     * A conversion constructor to allow for casting between Ptr types.
     */
    template <typename U>
    Ptr(Ptr<U>& other);

    /**
     * A conversion constructor to allow for casting between Ptr types.
     */
    template <typename U>
    Ptr(const Ptr<U>& other);

    /**
     * Destroy a Ptr.  Typically happens when Ptr objects go out of language
     * scope.
     */
    ~Ptr();

    /**
     * Assignment operator.  Any previously set object has its reference count
     * decremented, and the new object is acquired by incrementing its reference
     * count.
     */
    Ptr<T>& operator=(Ptr const& other);

    /**
     * The point operator, which is the heart of a smart pointer class.
     */
    T* operator->();

    /**
     * The dereference operator, which allows getting a reference to the Ptr
     * type.
     */
    T& operator*();

    /**
     * Get the underlying object pointer.  You must never delete this pointer
     * yourself.
     */
    T* Peek();

  private:
    /**
     * The actual pointer to the underlying object which the Ptr manages.
     */
    T* ptr;
};

template <typename T>
Ptr<T>::Ptr() : ptr(NULL)
{
}

template <typename T>
Ptr<T>::Ptr(T* p) : ptr(p)
{
    if (ptr) {
        ptr->IncRef();
    }
}

template <typename T>
template <typename U>
Ptr<T>::Ptr(Ptr<U>& other)
    : ptr(other.Peek())
{
    if (ptr) {
        ptr->IncRef();
    }
}

template <typename T>
template <typename U>
Ptr<T>::Ptr(const Ptr<U>& other)
    : ptr(other.Peek())
{
    if (ptr) {
        ptr->IncRef();
    }
}

template <typename T>
Ptr<T>::~Ptr()
{
    if (ptr) {
        ptr->DecRef();
    }
}

template <typename T>
Ptr<T>& Ptr<T>::operator=(Ptr const& other)
{
    //
    // If assigning this Ptr to itself, we don't increment the reference count
    // since nothing has changed in that case.
    //
    if (this == &other) {
        return *this;
    }

    //
    // If this Ptr is already pointing to a reference counted object, then
    // we need to decrement that reference count to reflect a possible change
    // in pointer.
    //
    if (ptr) {
        ptr->DecRef();
    }

    //
    // Initialize our pointer to point to the object pointed to by the other
    // Ptr.
    //
    ptr = other.ptr;

    //
    // If the new pointer is not NULL, then we need to increment the reference
    // count to reflect our interest in the object.
    //
    if (ptr) {
        ptr->IncRef();
    }

    return *this;
}

template <typename T>
T* Ptr<T>::operator->()
{
    return ptr;
}

template <typename T>
T& Ptr<T>::operator*()
{
    return *ptr;
}

template <typename T>
T* Ptr<T>::Peek(void)
{
    return ptr;
}

//
// What follows are a number of convenience functions that allow new objects to
// be created and assigned to new Ptr instances.  We do this via non-member
// functions to avoid adding single argument constructors.  A singe argument
// constructor makes conversion operators and conversion constructors
// ambiguous and prevents us from casting Ptr from one flavor to another.
//
// To use, do something like:
//
//     Ptr<MyType> ptr = NewPtr<MyType>(arg1, arg2, ..., argN);
//
template <typename T>
Ptr<T> NewPtr(void)
{
    return Ptr<T>(new T());
}

template <typename T, typename A1>
Ptr<T> NewPtr(A1 arg1)
{
    return Ptr<T>(new T(arg1));
}

template <typename T, typename A1, typename A2>
Ptr<T> NewPtr(A1 arg1, A2 arg2)
{
    return Ptr<T>(new T(arg1, arg2));
}

template <typename T, typename A1, typename A2, typename A3>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3)
{
    return Ptr<T>(new T(arg1, arg2, arg3));
}

template <typename T, typename A1, typename A2, typename A3, typename A4>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3, A4 arg4)
{
    return Ptr<T>(new T(arg1, arg2, arg3, arg4));
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5)
{
    return Ptr<T>(new T(arg1, arg2, arg3, arg4, arg5));
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6)
{
    return Ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6));
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6, A7 arg7)
{
    return Ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
Ptr<T> NewPtr(A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5, A6 arg6, A7 arg7, A8 arg8)
{
    return Ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
}

class RefCountBase {
  public:

    RefCountBase() : refCount(0) { }
    virtual ~RefCountBase() { }

    void IncRef(void)
    {
        IncrementAndFetch(&refCount);
    }

    void DecRef(void)
    {
        if (DecrementAndFetch(&refCount) == 0) {
            delete this;
        }
    }

  private:
    volatile mutable int32_t refCount;
};

} // namespace qcc

#endif // _QCC_PTR_H
