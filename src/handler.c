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

#include "handler.h"
#include "falcon.h"

typedef struct {
	falcon_handler_func func;
	gpointer userdata;
} falcon_handler_t;

static GMutex *lock = NULL;
static GHashTable *registry = NULL;

static void
falcon_handler_created_event(falcon_object_t *object,
                             falcon_event_code_t event ATTRIBUTE_UNUSED,
                             falcon_cache_t *cache) {
	if (!falcon_cache_add(cache, object))
		g_warning(_("Failed to add %s to the cache."),
		          falcon_object_get_name(object));
}

static void
falcon_handler_deleted_event(falcon_object_t *object,
                             falcon_event_code_t event ATTRIBUTE_UNUSED,
                             falcon_cache_t *cache) {
	if (!falcon_cache_delete(cache, falcon_object_get_name(object)))
		g_warning(_("Failed to delete %s from the cache."),
		          falcon_object_get_name(object));
}

static void
falcon_handler_changed_event(falcon_object_t *object,
                             falcon_event_code_t event ATTRIBUTE_UNUSED,
                             falcon_cache_t *cache) {
	if (!falcon_cache_add(cache, object))
		g_warning(_("Failed to change %s in the cache."),
		          falcon_object_get_name(object));
}

static inline gint falcon_handler_compare(gconstpointer a, gconstpointer b) {
	const falcon_handler_t *handler = (const falcon_handler_t *)a;
	const falcon_handler_func func = (const falcon_handler_func)b;

	return handler->func == func ? 0 : -1;
}

gboolean falcon_handler_register(falcon_event_code_t events,
                                 falcon_handler_func func, gpointer userdata) {
	GHashTableIter iter;
	gpointer key;
	GSList *list = NULL;
	falcon_event_code_t event = EVENT_NONE;
	falcon_handler_t *value = NULL;

	g_mutex_lock(lock);
	if (!registry) {
		g_critical(_("Registry uninitialized."
		             " Failed to register handler for event %s"),
		           falcon_event_to_string(event));
		g_mutex_unlock(lock);
		return FALSE;
	}

	g_hash_table_iter_init(&iter, registry);
	while (g_hash_table_iter_next(&iter, &key, (gpointer *)&list)) {
		event = GPOINTER_TO_UINT(key);
		if ((events & event) == event) {
			value = g_new0(falcon_handler_t, 1);
			value->func = func;
			value->userdata = userdata;
			list = g_slist_append(list, value);
			g_hash_table_insert(registry, key, list);
		}
	}

	g_mutex_unlock(lock);
	return TRUE;
}

gboolean falcon_handler_unregister(falcon_event_code_t events,
                                   falcon_handler_func func) {
	GHashTableIter iter;
	gpointer key;
	GSList *list = NULL;
	GSList *target = NULL;
	falcon_event_code_t event = EVENT_NONE;

	g_mutex_lock(lock);
	if (!registry) {
		g_critical(_("Registry uninitialized."
		             " Failed to unregister handler for event %s"),
		           falcon_event_to_string(event));
		g_mutex_unlock(lock);
		return FALSE;
	}

	g_hash_table_iter_init(&iter, registry);
	while (g_hash_table_iter_next(&iter, &key, (gpointer *)&list)) {
		event = GPOINTER_TO_UINT(key);
		if (list && (events & event) == event) {
			target = g_slist_find_custom(list, func, falcon_handler_compare);
			if (target) {
				g_free(target->data);
				list = g_slist_delete_link(list, target);
				g_hash_table_insert(registry, key, list);
			}
		}
	}

	g_mutex_unlock(lock);
	return TRUE;
}

void falcon_handler_registry_init(void) {
	g_return_if_fail(!lock);
	g_return_if_fail(!registry);

	lock = g_mutex_new();
	registry = g_hash_table_new(g_direct_hash, g_direct_equal);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_DIR_CREATED), NULL);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_DIR_DELETED), NULL);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_DIR_CHANGED), NULL);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_FILE_CREATED), NULL);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_FILE_DELETED), NULL);
	g_hash_table_insert(registry, GUINT_TO_POINTER(EVENT_FILE_CHANGED), NULL);
}

void falcon_handler_registry_shutdown(void) {
	GHashTableIter iter;
	gpointer key, value;
	GSList *list = NULL;

	g_return_if_fail(lock);
	g_return_if_fail(registry);

	g_mutex_lock(lock);
	g_hash_table_iter_init (&iter, registry);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		for (list = (GSList *)value; list; list = g_slist_next(list))
			g_free(list->data);
		g_slist_free((GSList *)value);
	}

	g_hash_table_unref(registry);
	g_mutex_unlock(lock);

	g_mutex_free(lock);
}

void falcon_handler(falcon_object_t *object, falcon_event_code_t event,
                    falcon_cache_t *cache) {
	falcon_handler_t *value = NULL;
	GSList *list = NULL;
	GSList *cur = NULL;
	GSList *prev = NULL;

	g_message(_("Handling %s for event %s."), falcon_object_get_name(object),
	          falcon_event_to_string(event));

	g_mutex_lock(lock);
	/* Call user handlers. */
	if (!registry) {
		g_critical(_("Registry uninitialized. Failed to handle event %s"),
		           falcon_event_to_string(event));
		g_mutex_unlock(lock);
		return;
	}

	list = g_hash_table_lookup(registry, GUINT_TO_POINTER(event));

	prev = list;
	for (cur = prev; cur; cur = g_slist_next(prev)) {
		value = (falcon_handler_t *)cur->data;
		if (!value->func(object, event, value->userdata)) {
			list = g_slist_delete_link(list, cur);
			if (cur == prev)
				prev = list;
		} else {
			prev = cur;
		}
	}

	g_hash_table_insert(registry, GUINT_TO_POINTER(event), list);
	g_mutex_unlock(lock);

	/* Update cache. */
	switch (event) {
	case EVENT_DIR_CREATED:
	case EVENT_FILE_CREATED:
		falcon_handler_created_event(object, event, cache);
		break;
	case EVENT_DIR_DELETED:
	case EVENT_FILE_DELETED:
		falcon_handler_deleted_event(object, event, cache);
		break;
	case EVENT_DIR_CHANGED:
	case EVENT_FILE_CHANGED:
		falcon_handler_changed_event(object, event, cache);
		break;
	default:
		break;
	}
}
