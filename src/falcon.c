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

#include "falcon.h"

typedef struct {
	GMutex lock;
	GQueue pending_objects;
	GQueue failed_objects;
	falcon_cache_t *cache;
} falcon_context_t;

falcon_context_t falcon_context;

void falcon_context_init(void) {
	g_queue_init(&falcon_context.pending_objects);
	g_queue_init(&falcon_context.failed_objects);
	falcon_context.cache = falcon_cache_new();
}

void falcon_context_free(void) {
	falcon_object_t *object = NULL;

	while ((object = g_queue_pop_head(falcon_context.pending_objects))) {
		falcon_object_free(object);
	}
	while ((object = g_queue_pop_head(falcon_context.failed_objects))) {
		falcon_object_free(object);
	}

	falcon_cache_free(falcon_context.cache);
}

void falcon_push(GQueue *queue, falcon_object_t *object) {
	GList *l = NULL;

	g_return_if_fail(queue);
	g_return_if_fail(object);

	g_mutex_lock(&falcon_context.lock);
	l = g_queue_find_custom(queue, object->name);
	if (!l)
		g_queue_push_tail(queue, object);
	g_mutex_unlock(&falcon_context.lock);
}

void falcon_task_add(falcon_object_t *object) {
	falcon_push(&falcon_context.pending_objects, object);
}

void falcon_failed_add(falcon_object_t *object) {
	falcon_push(&falcon_context.failed_objects, object);
}
