/*
 * std.c
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
#include <stdlib.h>
#include <stdint.h>

#include <std.h>

uint32_t heapBytesAllocated = 0;

void *mmalloc(size_t size) {
	/* void *p = malloc(size);
	if (p != NULL) {
		heapBytesAllocated += size;
	}
	return p; */
	return mcalloc(size, 1); // Clear memory to zero!
}

void *mrealloc(void *ptr, size_t newSize, size_t oldSize) {
	void *p = realloc(ptr, newSize);
	if (p != NULL) {
		heapBytesAllocated += newSize;
		heapBytesAllocated -= oldSize;
	}
	return p;
}

void *mcalloc(size_t n, size_t s) {
	void *p = calloc(n, s);
	if (p != NULL) {
		heapBytesAllocated += (n * s);
	}
	return p;
}

void mfree(void *ptr, size_t size) {
	free(ptr);
	heapBytesAllocated -= size;
}
