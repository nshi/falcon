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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glib/gstdio.h>

#include "cache.h"

struct falcon_cache_st {
	GMutex *lock;
	guint64 count;
	trie_node_t *objects;
};

static void falcon_cache_recursive_foreach_top(const trie_node_t *node,
                                               GFunc func, gpointer udata) {
	falcon_object_t *data = NULL;

	while (node) {
		if ((data = trie_data(node))) {
			func(data, udata);
		} else if (trie_child(node)) {
			falcon_cache_recursive_foreach_top(trie_child(node), func, udata);
		}
		node = trie_next(node);
	}
}

static void falcon_cache_recursive_foreach_descendant(const trie_node_t *node,
                                                      GFunc func,
                                                      gpointer udata) {
	falcon_object_t *data = NULL;

	while (node) {
		if (trie_child(node))
			falcon_cache_recursive_foreach_descendant(trie_child(node), func,
			                                          udata);
		if ((data = trie_data(node)))
			func(data, udata);
		node = trie_next(node);
	}
}

static void recursive_print(const trie_node_t *node, guint l)
{
	guint i = 0;
	guint len = 0;

	while (node) {
		printf("%s", trie_key(node) ? trie_key(node) : "ROOT");
		if (trie_child(node)) {
			if (trie_key(node))
				len = strlen(trie_key(node)) + 2;
			printf("->");
			recursive_print(trie_child(node), l + len);
		}
		if (trie_next(node))
			printf("\n");
		for (i = 0; i < l; i++)
			printf(" ");
		node = trie_next(node);
	}
}

falcon_cache_t *falcon_cache_new(void)
{
	falcon_cache_t *cache = g_new0(falcon_cache_t, 1);
	cache->lock = g_mutex_new();
	cache->objects = trie_new(G_DIR_SEPARATOR_S, 1);
	return cache;
}

void falcon_cache_free(falcon_cache_t *cache)
{
	g_return_if_fail(cache);

	trie_free(cache->objects, (trie_free_func)falcon_object_free);
	g_mutex_free(cache->lock);
	g_free(cache);
}

falcon_object_t *falcon_cache_get(falcon_cache_t *cache, const gchar *name)
{
	trie_node_t *node = NULL;

	g_return_val_if_fail(cache, NULL);
	g_return_val_if_fail(name, NULL);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	g_mutex_unlock(cache->lock);
	return trie_data(node);
}

gboolean falcon_cache_add(falcon_cache_t *cache, falcon_object_t *object)
{
	trie_node_t *old_node = NULL;
	falcon_object_t *old = NULL;
	falcon_object_t *dup = falcon_object_copy(object);

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(object, FALSE);

	g_mutex_lock(cache->lock);
	old_node = trie_find(cache->objects, falcon_object_get_name(dup));

	if (old_node && (old = trie_data(old_node))) {
		falcon_object_free(old);
		trie_set_data(old_node, dup);
	} else {
		trie_add(cache->objects, falcon_object_get_name(dup), dup);
		cache->count++;
	}

	g_mutex_unlock(cache->lock);

	return TRUE;
}

gboolean falcon_cache_delete(falcon_cache_t *cache, const gchar *name)
{
	trie_node_t *node = NULL;

	g_return_val_if_fail(cache, FALSE);
	g_return_val_if_fail(name, FALSE);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	if (!node) {
		g_mutex_unlock(cache->lock);
		g_warning(_("Failed to delete \"%s\", it does not exist in the cache."),
		          name);
		return FALSE;
	}

	trie_delete(cache->objects, name, (trie_free_func)falcon_object_free);
	cache->count--;
	g_mutex_unlock(cache->lock);

	return TRUE;
}

void falcon_cache_clear(falcon_cache_t *cache)
{
	g_return_if_fail(cache);

	g_mutex_lock(cache->lock);
	trie_free(cache->objects, (trie_free_func)falcon_object_free);
	cache->objects = trie_new(G_DIR_SEPARATOR_S, 1);
	cache->count = 0;
	g_mutex_unlock(cache->lock);
}

void falcon_cache_foreach_top(falcon_cache_t *cache, GFunc func,
                              gpointer userdata) {
	g_return_if_fail(cache);
	g_return_if_fail(func);

	g_mutex_lock(cache->lock);
	falcon_cache_recursive_foreach_top(trie_child(cache->objects), func,
	                                   userdata);
	g_mutex_unlock(cache->lock);
}

void falcon_cache_foreach_child(falcon_cache_t *cache, const gchar *name,
                                GFunc func, gpointer userdata) {
	trie_node_t *node = NULL;
	trie_node_t *next = NULL;
	falcon_object_t *data = NULL;

	g_return_if_fail(cache);
	g_return_if_fail(func);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	next = trie_child(node);
	while (next) {
		if ((data = trie_data(next)))
			func(data, userdata);
		next = trie_next(next);
	}
	if ((data = trie_data(node)))
		func(data, userdata);
	g_mutex_unlock(cache->lock);
}

void falcon_cache_foreach_descendant(falcon_cache_t *cache, const gchar *name,
                                     GFunc func, gpointer userdata) {
	trie_node_t *node = NULL;
	falcon_object_t *data = NULL;

	g_return_if_fail(cache);
	g_return_if_fail(func);

	g_mutex_lock(cache->lock);
	node = trie_find(cache->objects, name);
	falcon_cache_recursive_foreach_descendant(trie_child(node), func, userdata);
	if ((data = trie_data(node)))
		func(data, userdata);
	g_mutex_unlock(cache->lock);
}

/*
 * Cache file format
 *
 * All values are stored in Big Endian order.
 * 8 bytes unsigned integer: number of objects following.
 * For each object,
 * 2 bytes unsigned integer: name length, follow by name string.
 * 8 bytes signed integer: file size
 * 8 bytes signed integer: time
 * 4 bytes unsigned integer: mode
 * 1 byte unsigned integer: watchability flag.
 */
gboolean falcon_cache_load(falcon_cache_t *cache, const gchar *name)
{
	int fd = 0;
	guint64 count = 0;
	falcon_object_t *object = NULL;
	guint64 i;
	gboolean ret = TRUE;

	g_return_val_if_fail(cache, FALSE);
	if (!name)
		return FALSE;

	fd = g_open(name, O_RDONLY, 0);
	if (fd == -1) {
		g_critical(_("Failed to read cache file %s: %s"), name,
		           g_strerror(errno));
		return FALSE;
	}

	if (read(fd, &(count), 8) == -1) {
		g_critical(_("Failed to read cache file %s: %s"), name,
		           g_strerror(errno));
		close(fd);
		return FALSE;
	}
	count = GUINT64_FROM_BE(count);

	g_debug(_("Loaded %lu cache keys."), count);

	for (i = 1; i <= count; i++) {
		object = falcon_object_new(NULL);
		if (!falcon_object_load(object, GINT_TO_POINTER(fd))) {
			g_critical(_("Failed to load object %lu"), i);
			falcon_object_free(object);
			ret = FALSE;
			break;
		}

		g_debug(_("Loaded object \"%s\": dir=%s, size=%ld, time=%ld, watch=%s"),
		        falcon_object_get_name(object),
		        falcon_object_isdir(object) ? "yes" : "no",
		        falcon_object_get_size(object),
		        falcon_object_get_time(object),
		        falcon_object_get_watch(object) ? "yes" : "no");

		falcon_cache_add(cache, object);

		falcon_object_free(object);
		object = NULL;
	}

	close(fd);

	return ret;
}

gboolean falcon_cache_save(const falcon_cache_t *cache, const gchar *name)
{
	int fd = 0;
	guint64 count = 0;

	g_return_val_if_fail(cache, FALSE);
	if (!name)
		return FALSE;

	fd = g_open(name, O_WRONLY | O_TRUNC | O_CREAT,
	            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		g_critical(_("Failed to open cache file %s: %s"), name,
		           g_strerror(errno));
		return FALSE;
	}

	g_mutex_lock(cache->lock);
	count = GUINT64_TO_BE(cache->count);
	write(fd, &count, 8);
	trie_foreach(cache->objects, falcon_object_save, GINT_TO_POINTER(fd));
	g_mutex_unlock(cache->lock);
	close(fd);

	return TRUE;
}

void falcon_cache_print(const falcon_cache_t *cache)
{
	recursive_print(cache->objects, 6);
}
