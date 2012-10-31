/*
 * tasks.c
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

#define DEBUG 1

#include <std.h>
#include <tasks.h>
#include <net/controller.h>

typedef struct TaskElement TaskElement;
struct TaskElement {
    Task task;
    TestFunc test;
    TaskElement *next;
#if DEBUG >= 1
    char *name;
#endif
};

TaskElement *taskList = NULL;

uint8_t tasksRegistered(void) {
    uint8_t c = 0;
    for (TaskElement *p = taskList; p != NULL; p = p->next) {
        c++;
    }
    return c;
}

uint8_t addTask(Task func, TestFunc testFunc, char *name) {
    TaskElement *p = (TaskElement *)mmalloc(sizeof(TaskElement));
    if (p == NULL) {
        return 1;
    }
    p->task = func;
    p->test = testFunc;
    p->next = taskList;
#if DEBUG >= 1
    p->name = name;
#endif
    taskList = p;
    return 0;
}

void tasks(void) {
    for (TaskElement *p = taskList; p != NULL; p = p->next) {
        if ((p->test == NULL) || (p->test() != 0)) {
            p->task();
#if DEBUG >= 1
            debugPrint("Executed ");
            debugPrint(p->name);
            debugPrint("\n");
#endif
        }
    }
}
