/* lock_guard.h                  -*-C++-*-
 *
 *************************************************************************
 *
 * Copyright (C) 2011-2016, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * *********************************************************************
 * 
 * PLEASE NOTE: This file is a downstream copy of a file mainitained in
 * a repository at cilkplus.org. Changes made to this file that are not
 * submitted through the contribution process detailed at
 * http://www.cilkplus.org/submit-cilk-contribution will be lost the next
 * time that a new version is released. Changes only submitted to the
 * GNU compiler collection or posted to the git repository at
 * https://bitbucket.org/intelcilkruntime/intel-cilk-runtime.git are
 * not tracked.
 * 
 * We welcome your contributions to this open source project. Thank you
 * for your assistance in helping us improve Cilk Plus.
 *
 **************************************************************************
 *
 * Lock guard patterned after the std::lock_guard class template proposed in
 * the C++ 0x draft standard.
 *
 * An object of type lock_guard controls the ownership of a mutex object
 * within a scope. A lock_guard object maintains ownership of a mutex object
 * throughout the lock_guard object's lifetime. The behavior of a program is
 * undefined if the mutex referenced by pm does not exist for the entire
 * lifetime of the lock_guard object.
 */

#ifndef LOCK_GUARD_H_INCLUDED
#define LOCK_GUARD_H_INCLUDED

#include <cilk/cilk.h>

namespace cilkscreen
{
    template <class Mutex>
    class lock_guard
    {
    public:
        typedef Mutex mutex_type;

        explicit lock_guard(mutex_type &m) : pm(m)
        {
            pm.lock();
            locked = true;
        }

        ~lock_guard()
        {
            locked = false;
            pm.unlock();
        }

    private:
        lock_guard(lock_guard const&);
        lock_guard& operator=(lock_guard const&);

    private:
        // exposition only:
        mutex_type &pm;
        bool locked;
    };
}

#endif  // LOCK_GUARD_H_INCLUDED
