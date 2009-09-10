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

#ifndef _WALKER_H_
#define _WALKER_H_

#include "common.h"
#include "cache.h"

#define FALCON_WALKER_ERROR g_quark_from_static_string("falcon-walker-error")

typedef struct {
	falcon_cache_t *cache;
	GAsyncQueue *queue;
	void (*falcon_walker_return)(GQueue *objects, GError *error);
} falcon_walker_context_t;

/*
 * This starts the walker thread.
 *
 * A walker thread walks the list of paths given in the walker_data. It only
 * looks at the content of a given path and compares it with the one in the
 * cache. If the path is a directory, it will read the list of files under this
 * directory and insert them into a queue for other threads to handle. It does
 * not recursively go down into the sub-directories.
 *
 * The walker assumes that all the paths passed to it exist. If not found, an
 * EVENT_*_DELETED signal will be emmitted.
 *
 * The walker may not consume all the paths provided to it due to errors. The
 * caller should check if the given list of paths has been fully consumed after
 * the thread returns.
 */
void falcon_walker_run(gpointer data, gpointer userdata);

#endif
