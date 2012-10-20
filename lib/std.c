/*
 * std.c
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
#include <stdlib.h>
#include <stdint.h>

#define DEBUG 0

#include <std.h>
#include <net/controller.h> // Maybe DISABLE_HEAP_LOG is defined here?

uint32_t heapBytesAllocated = 0;

#ifndef DISABLE_HEAP_LOG

void *mmalloc(size_t size) {
    void *p = malloc(size);
    if (p != NULL) {
        heapBytesAllocated += (size);
#if DEBUG >= 1
        debugPrint("  + ");
        debugPrint(timeToString(size));
        debugPrint("\n");
#endif
    }
    return p;
}

void *mrealloc(void *ptr, size_t newSize, size_t oldSize) {
    void *p = realloc(ptr, newSize);
    if (p != NULL) {
        heapBytesAllocated += newSize;
        heapBytesAllocated -= oldSize;
#if DEBUG >= 1
        if (newSize > oldSize) {
            debugPrint("  + ");
            debugPrint(timeToString(newSize - oldSize));
        } else {
            debugPrint("  - ");
            debugPrint(timeToString(oldSize - newSize));
        }
        debugPrint("\n");
#endif
    }
    return p;
}

void *mcalloc(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (p != NULL) {
        heapBytesAllocated += (n * s);
#if DEBUG >= 1
        debugPrint("  + ");
        debugPrint(timeToString(n * s));
        debugPrint("\n");
#endif
    }
    return p;
}

void mfree(void *ptr, size_t size) {
    free(ptr);
    heapBytesAllocated -= size;
#if DEBUG >= 1
    debugPrint("  - ");
    debugPrint(timeToString(size));
    debugPrint("\n");
#endif
}

#else // DISABLE_HEAP_LOG defined

inline void *mmalloc(size_t size) {
    return malloc(size);
}

inline void *mrealloc(void *ptr, size_t newSize, size_t oldSize) {
    return realloc(ptr, newSize);
}

inline void *mcalloc(size_t n, size_t s) {
    return calloc(n, s);
}

inline void mfree(void *ptr, size_t size) {
    free(ptr);
}

#endif // DISABLE_HEAP_LOG
