/*
 * debug.h
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
#ifndef _DEBUG_H
#define _DEBUG_H

#include <avr/wdt.h>

// #define DISABLE_HEAP_LOG // Uncomment to disable counting allocated bytes

#define DEBUGOUT(x) serialWriteString(x) // Debug Output Function

// assert Implementation
#define ASSERTFUNC(x) ({ 							\
	if (!(x)) {										\
		if (DEBUG != 0) {							\
			debugPrint("\nError: ");				\
			debugPrint(__FILE__);					\
			debugPrint(":");						\
			debugPrint(timeToString(__LINE__));		\
			debugPrint(" ");						\
			debugPrint(__func__);					\
			debugPrint(": Assertion '");			\
			debugPrint(#x);							\
			debugPrint("' failed.\n");				\
			debugPrint("Reset in 2 Seconds.\n\n");	\
			wdt_enable(WDTO_2S);					\
			while(1);								\
		} else {									\
			wdt_enable(WDTO_15MS);					\
			while(1);								\
		}											\
	}												\
})

// Macro Magic
// Controls Debug Output with DEBUG definition.
// Define DEBUG before including this file!
// Disables all debug output if NDEBUG is defined

#ifdef NODEBUG
#define NDEBUG NODEBUG
#endif

#if (!(defined(NDEBUG)))
#define assert(x) ASSERTFUNC(x)
#if DEBUG >= 1
#define debugPrint(x) DEBUGOUT(x)
#else
#define debugPrint(ignore)
#endif // DEBUG >= 1
#else // NDEBUG defined
#define assert(ignore)
#define debugPrint(ignore)
#endif // ! defined NDEBUG

#endif // _DEBUG_H
