/*
 * debug.h
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _DEBUG_H
#define _DEBUG_H

#include <avr/wdt.h>

// #define DISABLE_HEAP_LOG // Uncomment to disable counting allocated bytes
// #define NDEBUG // Uncomment to disable debug and assert output

#define DEBUGOUT(x) serialWriteString(x) // Debug Output Function

// assert Implementation
#define ASSERTFUNC(x) ({                            \
    if (!(x)) {                                     \
        if (DEBUG != 0) {                           \
            debugPrint("\nError: ");                \
            debugPrint(__FILE__);                   \
            debugPrint(":");                        \
            debugPrint(timeToString(__LINE__));     \
            debugPrint(" ");                        \
            debugPrint(__func__);                   \
            debugPrint(": Assertion '");            \
            debugPrint(#x);                         \
            debugPrint("' failed.\n");              \
            debugPrint("Reset in 2 Seconds.\n\n");  \
            wdt_enable(WDTO_2S);                    \
            while(1);                               \
        } else {                                    \
            wdt_enable(WDTO_15MS);                  \
            while(1);                               \
        }                                           \
    }                                               \
})

// Macro Magic
// Controls Debug Output with DEBUG definition.
// Define DEBUG before including this file!
// Disables all debug output if NDEBUG is defined

#ifdef NODEBUG // Allow NODEBUG and NDEBUG
#define NDEBUG NODEBUG
#endif

#if (!(defined(NDEBUG))) && (DEBUG >= 1)
#define assert(x) ASSERTFUNC(x)
#define debugPrint(x) DEBUGOUT(x)
#else
#define assert(ignore)
#define debugPrint(ignore)
#endif // ! defined NDEBUG

#endif // _DEBUG_H
