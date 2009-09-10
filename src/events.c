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

#include <glib.h>

#include "events.h"

const gchar *falcon_event_to_string(falcon_event_code_t event) {
	switch (event) {
	case EVENT_DIR_CREATED:
		return "EVENT_DIR_CREATED";
		break;
	case EVENT_DIR_DELETED:
		return "EVENT_DIR_DELETED";
		break;
	case EVENT_DIR_CHANGED:
		return "EVENT_DIR_CHANGED";
		break;
	case EVENT_DIR_MOVED:
		return "EVENT_DIR_MOVED";
		break;
	case EVENT_FILE_CREATED:
		return "EVENT_FILE_CREATED";
		break;
	case EVENT_FILE_DELETED:
		return "EVENT_FILE_DELETED";
		break;
	case EVENT_FILE_CHANGED:
		return "EVENT_FILE_CHANGED";
		break;
	case EVENT_FILE_MOVED:
		return "EVENT_FILE_MOVED";
		break;
	default:
		return "Unknown";
	}
}
