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

static void falcon_cache_save_one(trie_node_t *node, void *userdata) {
	falcon_object_t *object = (falcon_object_t *)trie_data(node);
	GKeyFile *file = (GKeyFile *)userdata;
	gint values[] = {object->mode, object->size, object->time, object->watch};
	g_key_file_set_integer_list(file, "falcon", object->name, values, 4);
}

static void recursive_print(const trie_node_t *node, guint l) {
	const trie_node_t *cur = node;
	guint i = 0;
	guint len = 0;

	while (cur) {
		printf("%s", trie_key(cur) ? trie_key(cur) : "ROOT");
		if (trie_child(cur)) {
			if (trie_key(cur))
				len = strlen(trie_key(cur)) + 2;
			printf("->");
			recursive_print(trie_child(cur), l + len);
		}
		if (trie_next(cur))
			printf("\n");
		for (i = 0; i < l; i++)
			printf(" ");
		cur = trie_next(cur);
	}
}

falcon_cache_t *falcon_cache_new(void) {
	falcon_cache_t *cache = g_new0(falcon_cache_t, 1);
	cache->lock = g_mutex_new();
	cache->objects = trie_new(G_DIR_SEPARATOR_S, 1);
	return cache;
}

void falcon_cache_free(falcon_cache_t *cache) {
	g_return_if_fail(cache);

	trie_free(cache->objects, (trie_free_func)falcon_object_free);
	g_mutex_free(cache->lock);
	g_free(cache);
}

falcon_object_t *falcon_cache_get(falcon_cache_t *cache, const gchar *name) {
	trie_node_t *node = NULL;

	g_return_val_if_fail(cache, NULL);
	g_return_val_if_fail(name, NULL);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	g_mutex_unlock(cache->lock);
	return trie_data(node);
}

gboolean falcon_cache_add(falcon_cache_t *cache, falcon_object_t *object) {
	trie_node_t *old_node = NULL;
	falcon_object_t *old = NULL;
	falcon_object_t *dup = falcon_object_copy(object);

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(object, FALSE);

	g_mutex_lock(cache->lock);
	old_node = trie_find(cache->objects, dup->name);

	if (old_node && (old = trie_data(old_node))) {
		falcon_object_free(old);
		trie_set_data(old_node, dup);
	} else {
		trie_add(cache->objects, dup->name, dup);
	}

	g_mutex_unlock(cache->lock);

	return TRUE;
}

gboolean falcon_cache_delete(falcon_cache_t *cache, const gchar *name) {
	trie_node_t *node = NULL;

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(name, FALSE);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	if (!node) {
		g_mutex_unlock(cache->lock);
		g_warning(_("Deleting \"%s\" from cache before creation."), name);
		return FALSE;
	}

	trie_delete(cache->objects, name, (trie_free_func)falcon_object_free);
	g_mutex_unlock(cache->lock);

	return TRUE;
}

void falcon_cache_top_foreach(falcon_cache_t *cache, GFunc func,
                              gpointer userdata) {
	trie_node_t *node = NULL;

	g_return_if_fail(cache);
	g_return_if_fail(func);

	g_mutex_lock(cache->lock);
	for (node = trie_child(cache->objects); node; node = trie_next(node))
		func(trie_data(node), userdata);
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
	trie_foreach(cache->objects, falcon_cache_save_one, file);
	g_mutex_unlock(cache->lock);

	data = g_key_file_to_data(file, &size, NULL);
	output = g_fopen(name, "w");
	fprintf(output, data);
	fclose(output);

	g_free(data);
	g_key_file_free(file);

	return TRUE;
}

void falcon_cache_print(const falcon_cache_t *cache) {
	recursive_print(cache->objects, 6);
}
