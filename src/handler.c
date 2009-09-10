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

GHashTable *falcon_handler_registry = NULL;

void falcon_handler_created_event(falcon_object_t *object,
                                  falcon_event_code_t event,
                                  falcon_cache_t *cache) {
	if (!falcon_cache_add_object(cache, object)) {
		g_warning(_("Failed to add %s to the cache."), object->name);
		return;
	}

	if (event == EVENT_DIR_CREATED)
		falcon_add_task(object);
}

void falcon_handler_deleted_event(falcon_object_t *object,
                                  falcon_event_code_t event,
                                  falcon_cache_t *cache) {
	if (!falcon_cache_delete_object(cache, object->name, TRUE))
		g_warning(_("Failed to delete %s from the cache."), object->name);
}

void falcon_handler_changed_event(falcon_object_t *object,
                                  falcon_event_code_t event,
                                  falcon_cache_t *cache) {

}

void falcon_handler_moved_event(falcon_object_t *object,
                                falcon_event_code_t event,
                                falcon_cache_t *cache) {

}

gboolean falcon_handler_register(falcon_event_code_t event,
                                 falcon_event_handler_func func) {
	GSList *list = NULL;

	if (!falcon_handler_registry) {
		g_critical(_("Registry uninitialized."
		             " Failed to register handler for event %s"),
		           falcon_event_to_string(event));
		return FALSE;
	}

	list = g_hash_table_lookup(falcon_handler_registry, GUINT_TO_POINTER(event));
	list = g_slist_append(list, func);
	g_hash_table_insert(falcon_handler_registry, GUINT_TO_POINTER(event), list);

	return TRUE;
}

gboolean falcon_handler_unregister(falcon_event_code_t event,
                                   falcon_event_handler_func func) {
	GSList *list = NULL;

	if (!falcon_handler_registry) {
		g_critical(_("Registry uninitialized."
		             " Failed to unregister handler for event %s"),
		           falcon_event_to_string(event));
		return FALSE;
	}

	list = g_hash_table_lookup(falcon_handler_registry, GUINT_TO_POINTER(event));
	g_return_val_if_fail(list, FALSE);
	list = g_slist_remove_all(list, func);
	g_hash_table_insert(falcon_handler_registry, GUINT_TO_POINTER(event), list);

	return TRUE;
}

void falcon_handler_registry_init(void) {
	g_return_if_fail(!falcon_handler_registry);

	falcon_handler_registry = g_hash_table_new(g_int_hash, g_int_equal);
}

void falcon_handler(falcon_object_t *object, falcon_event_code_t event,
                    falcon_cache_t *cache) {
	falcon_handler_func func = NULL;
	GSList *list = NULL;
	GSList *cur = NULL;
	GSList *prev = NULL;

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
	case EVENT_DIR_MOVED:
	case EVENT_FILE_MOVED:
		falcon_handler_moved_event(object, event, cache);
		break;
	default:
		break;
	}

	/* Call user handlers. */
	if (!falcon_handler_registry) {
		g_critical(_("Registry uninitialized. Failed to handle event %s"),
		           falcon_event_to_string(event));
		return;
	}

	list = g_hash_table_lookup(falcon_handler_registry, GUINT_TO_POINTER(event));
	g_return_if_fail(list);

	prev = list;
	for (cur = prev; cur; cur = g_slist_next(prev)) {
		if (!cur->data(object, event)) {
			list = g_slist_delete_link(list, cur);
			if (cur == prev)
				prev = list;
		} else {
			prev = cur;
		}
	}

	g_hash_table_insert(falcon_handler_registry, GUINT_TO_POINTER(event), list);
}
