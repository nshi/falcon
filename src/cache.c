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

#include <glib/gstdio.h>
#include <stdio.h>

#include "cache.h"

static gint falcon_cache_object_startswith(gconstpointer a, gconstpointer b) {
	return g_str_has_prefix(((const falcon_object_t *)a)->name,
	                        (const gchar *)b) ? 0 : -1;
}

static void falcon_cache_recursive_delete(falcon_cache_t *cache,
                                          const gchar *name) {
	GList *object = NULL;

	g_return_if_fail(cache);
	g_return_if_fail(name);

	do {
		object = g_queue_find_custom(cache->objects, name,
		                             falcon_cache_object_startswith);
		if (object) {
			g_queue_unlink(cache->objects, object);
			falcon_object_free(object->data);
		}
	} while (object);
}

static void falcon_cache_save_one(gpointer data, gpointer userdata) {
	falcon_object_t *object = (falcon_object_t *)data;
	GKeyFile *file = (GKeyFile *)userdata;
	gint values[] = {object->mode, object->size, object->time, object->watch};
	g_key_file_set_integer_list(file, "falcon", object->name, values, 4);
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

	while ((object = g_queue_pop_head(cache->objects)))
		falcon_object_free(object);
	g_queue_free(cache->objects);
	g_mutex_free(cache->lock);
	g_free(cache);
}

falcon_object_t *falcon_cache_get(falcon_cache_t *cache,
                                         const gchar *name) {
	GList *object = NULL;

	g_return_val_if_fail(cache, NULL);
	g_return_val_if_fail(name, NULL);

	g_mutex_lock(cache->lock);
	object = g_queue_find_custom(cache->objects, name, falcon_object_compare);
	g_mutex_unlock(cache->lock);
	return object ? object->data : NULL;
}

gboolean falcon_cache_add(falcon_cache_t *cache, falcon_object_t *object) {
	GList *l = NULL;
	falcon_object_t *dup = falcon_object_copy(object);

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(object, FALSE);

	g_mutex_lock(cache->lock);
	l = g_queue_find_custom(cache->objects, dup->name, falcon_object_compare);

	if (l) {
		falcon_object_free(l->data);
		l->data = dup;
	} else {
		g_queue_push_tail(cache->objects, dup);
	}

	g_mutex_unlock(cache->lock);

	return TRUE;
}

gboolean falcon_cache_delete(falcon_cache_t *cache, const gchar *name,
                             gboolean flag) {
	GList *l = NULL;

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(name, FALSE);

	g_mutex_lock(cache->lock);
	l = g_queue_find_custom(cache->objects, name, falcon_object_compare);
	if (!l) {
		g_mutex_unlock(cache->lock);
		g_warning(_("Deleting \"%s\" from cache before creation."), name);
		return FALSE;
	}

	g_queue_unlink(cache->objects, l);
	if (flag)
		falcon_cache_recursive_delete(cache, name);
	g_mutex_unlock(cache->lock);

	falcon_object_free(l->data);

	return TRUE;
}

void falcon_cache_foreach(falcon_cache_t *cache, GFunc func, gpointer userdata) {
	g_return_if_fail(cache);
	g_return_if_fail(func);

	g_mutex_lock(cache->lock);
	g_queue_foreach(cache->objects, func, userdata);
	g_mutex_unlock(cache->lock);
}

gboolean falcon_cache_load(falcon_cache_t *cache, const gchar *name) {
	GKeyFile *file = NULL;
	gchar **keys = NULL;
	gsize size = 0;
	gint *values = NULL;
	gsize vsize = 0;
	GError *error = NULL;
	falcon_object_t *object = NULL;
	gsize i;

	g_return_val_if_fail(cache, FALSE);
	if (!name)
		return FALSE;

	file = g_key_file_new();
	if (!g_key_file_load_from_file(file, name, G_KEY_FILE_NONE, &error)) {
		error->code = FALCON_ERROR_CRITICAL;
		falcon_error_report(error);
		g_error_free(error);
		g_key_file_free(file);
		return FALSE;
	}

	if (!(keys = g_key_file_get_keys(file, "falcon", &size, &error))) {
		error->code = FALCON_ERROR_CRITICAL;
		falcon_error_report(error);
		g_error_free(error);
		g_key_file_free(file);
		return FALSE;
	}
	g_debug(_("Loaded %ld cache keys."), size);

	for (i = 0; i < size; i++) {
		if (!(values = g_key_file_get_integer_list(file, "falcon",
		                                           keys[i], &vsize, &error))) {
			error->code = FALCON_ERROR_CRITICAL;
			falcon_error_report(error);
			g_error_free(error);
			g_key_file_free(file);
			return FALSE;
		}
		g_debug(_("Loading key %ld: %d, %d, %d, %d"),
		        i, values[0], values[1], values[2], values[3]);

		object = falcon_object_new(keys[i]);
		falcon_object_set_mode(object, values[0]);
		falcon_object_set_size(object, values[1]);
		falcon_object_set_time(object, values[2]);
		falcon_object_set_watch(object, values[3]);

		falcon_cache_add(cache, object);

		g_free(values);
		falcon_object_free(object);
	}

	g_strfreev(keys);
	g_key_file_free(file);

	return TRUE;
}

gboolean falcon_cache_save(const falcon_cache_t *cache, const gchar *name) {
	GKeyFile *file = NULL;
	gsize size = 0;
	FILE *output = NULL;
	gchar *data = NULL;

	g_return_val_if_fail(cache, FALSE);
	if (!name)
		return FALSE;

	file = g_key_file_new();
	g_mutex_lock(cache->lock);
	g_queue_foreach(cache->objects, falcon_cache_save_one, file);
	g_mutex_unlock(cache->lock);

	data = g_key_file_to_data(file, &size, NULL);
	output = g_fopen(name, "w");
	fprintf(output, data);
	fclose(output);

	g_free(data);
	g_key_file_free(file);

	return TRUE;
}
