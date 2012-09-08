/*
 * std.h
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of avrNetStack.
 *
 * avrNetStack is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * avrNetStack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with avrNetStack.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _std_h
#define _std_h

#include <stdlib.h>

extern uint32_t heapBytesAllocated; // Number of bytes allocated

void *mmalloc(size_t size); // Use like regular malloc
void *mrealloc(void *ptr, size_t newSize, size_t oldSize); // like realloc, but needs oldsize
void *mcalloc(size_t n, size_t s); // Use like regular calloc
void mfree(void *ptr, size_t size); // Use like free, but give it the size too

#endif
