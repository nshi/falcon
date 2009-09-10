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

#include "cache.h"

gint falcon_cache_object_startswith(const falcon_object_t *a, const gchar *b) {
	return g_string_has_prefix(a->name, b) ? 0 : -1;
}

void falcon_cache_recursive_delete(falcon_cache_t *cache, const gchar *name) {
	GList *object = NULL;

	g_return_if_fail(cache);
	g_return_if_fail(name);

	do {
		object = g_queue_find_custom(cache->objects, name,
		                             falcon_cache_object_startswith);
		if (object)
			g_queue_remove_all(cache->objects, object->data);
	} while (object);
}

falcon_cache_t *falcon_cache_new(void) {
	falcon_cache_t *cache = g_new0(falcon_cache_t, 1);
	cache->lock = g_mutex_new();
	cache->objects = g_queue_new();
	return cache;
}

void falcon_cache_free(falcon_cache_t *cache) {
	falcon_object_t *object = NULL;

	g_return_if_fail(cache);

	while (object = g_queue_pop_head(cache->objects)) {
		falcon_object_free(object);
	}
	g_queue_free(cache->objects);
	g_mutex_free(cache->lock);
	g_free(cache);
}

falcon_object_t *falcon_cache_get_object(falcon_cache_t *cache,
                                         const gchar *name) {
	GList *object = NULL;

	g_return_val_if_fail(cache, NULL);
	g_return_val_if_fail(name, NULL);

	g_mutex_lock(cache->lock);
	object = g_queue_find_custom(cache->objects, name, falcon_object_compare);
	g_mutex_unlock(cache->lock);
	return object ? object->data : NULL;
}

gboolean falcon_cache_add_object(falcon_cache_t *cache,
                                 falcon_object_t *object) {
	GList *l = NULL;

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(object, FALSE);

	g_mutex_lock(cache->lock);
	l = g_queue_find_custom(cache->objects, name, falcon_object_compare);

	if (l) {
		falcon_object_free(l->data);
		l->data = object;
	} else {
		g_queue_push_tail(cache->objects, object);
	}

	g_mutex_unlock(cache->lock);

	return TRUE;
}

gboolean falcon_cache_delete_object(falcon_cache_t *cache,
                                    const gchar *name,
                                    gboolean flag) {
	GList *l = NULL;

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(name, FALSE);

	g_mutex_lock(cache->lock);
	l = g_queue_find_custom(cache->objects, name, falcon_object_compare);
	g_queue_remove_all(cache->objects, l->data);
	if (flag)
		falcon_cache_recursive_delete(cache, name);
	g_mutex_unlock(cache->lock);

	return TRUE;
}
