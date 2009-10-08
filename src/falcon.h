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

#ifndef _FALCON_H_
#define _FALCON_H_

#include <glib.h>

#include "common.h"
#include "cache.h"
#include "walker.h"

void falcon_init(const gchar *name);
/* Waits for all the tasks to be finished if wait is TRUE. */
void falcon_shutdown(const gchar *name, gboolean wait);

/*
 * Adds an object with the given name to the cache. Watch the directory for
 * changes if watch is TRUE.
 */
gboolean falcon_add(const gchar *name, gboolean watch);
/*
 * Deletes an object with the given name as well as its descendants.
 *
 * Attention: this is a blocking function. It has to wait until all the
 * currently pending objects have been processed.
 */
gboolean falcon_delete(const gchar *name);
/*
 * Clears all objects.
 *
 * Attention: this is a blocking function. It has to wait until all the
 * currently pending objects have been processed.
 */
void falcon_clear(void);
/*
 * Sets the watchability flag of an object to watch.
 */
gboolean falcon_set_watch(const gchar *name, gboolean watch);

void falcon_task_add(falcon_object_t *object);
void falcon_failed_add(falcon_object_t *object);
void falcon_walker_return(GError *error);

#endif
