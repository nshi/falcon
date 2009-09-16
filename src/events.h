/*
 * The MIT License
 *
 * Copyright (c) 2009 Ning Shi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "common.h"

typedef enum {
	EVENT_NONE = 0,

	/* Directory */
	EVENT_DIR_CREATED = (1 << 0),
	EVENT_DIR_DELETED = (1 << 1),
	EVENT_DIR_CHANGED = (1 << 2),

	/* File */
	EVENT_FILE_CREATED = (1 << 3),
	EVENT_FILE_DELETED = (1 << 4),
	EVENT_FILE_CHANGED = (1 << 5),

	EVENT_DIR_ALL = (EVENT_DIR_CREATED | EVENT_DIR_DELETED | EVENT_DIR_CHANGED),
	EVENT_FILE_ALL = (EVENT_FILE_CREATED | EVENT_FILE_DELETED
	                  | EVENT_FILE_CHANGED),
	EVENT_ALL = (EVENT_DIR_ALL | EVENT_FILE_ALL)
} falcon_event_code_t;

inline const gchar *falcon_event_to_string(falcon_event_code_t event);

#endif
