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

#include "filter.h"

typedef struct {
	gboolean is_dir;
	falcon_filter_func func;
} falcon_filter_t;

static GMutex *lock = NULL;
static GHashTable *registry = NULL;

static GRegex *falcon_filter_regex_new(const gchar *pattern)
{
	GRegex *regex = NULL;
	GError *error = NULL;

	regex = g_regex_new(pattern, G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY,
	                    &error);
	if (!regex) {
		error->code = FALCON_ERROR_WARNING;
		falcon_error_report(error);
		g_error_free(error);
		return NULL;
	}

	return regex;
}

static inline guint falcon_filter_regex_hash(gconstpointer key)
{
	const GRegex *regex = (const GRegex *)key;
	return g_str_hash(g_regex_get_pattern(regex));
}

static inline gboolean falcon_filter_regex_equal(gconstpointer a,
                                                 gconstpointer b)
{
	const GRegex *regex_a = (const GRegex *)a;
	const GRegex *regex_b = (const GRegex *)b;
	return (g_strcmp0(g_regex_get_pattern(regex_a),
	                  g_regex_get_pattern(regex_b)) == 0);
}

static inline gint falcon_filter_compare(gconstpointer a, gconstpointer b)
{
	const falcon_filter_t *filter_a = (const falcon_filter_t *)a;
	const falcon_filter_t *filter_b = (const falcon_filter_t *)b;

	return (filter_a->is_dir == filter_b->is_dir
	        && filter_a->func == filter_b->func) ? 0 : -1;
}

gboolean falcon_filter_register(gboolean is_dir, const gchar *pattern,
                                falcon_filter_func func)
{
	GRegex *regex = NULL;
	GRegex *key = NULL;
	GSList *list = NULL;
	falcon_filter_t *value = NULL;

	if (!pattern) {
		g_warning(_("Failed to register filter with no pattern."));
		return FALSE;
	}

	regex = falcon_filter_regex_new(pattern);
	if (!regex)
		return FALSE;

	g_mutex_lock(lock);
	if (!registry) {
		g_critical(_("Filter registry uninitialized."
		             " Failed to register filter with pattern %s"), pattern);
		g_mutex_unlock(lock);
		return FALSE;
	}

	if (!g_hash_table_lookup_extended(registry, regex, (gpointer *)&key,
	                                  (gpointer *)&list)) {
		g_regex_ref(regex);
		key = regex;
	}

	value = g_new0(falcon_filter_t, 1);
	value->is_dir = is_dir;
	value->func = func;

	if (g_slist_find_custom(list, &value, falcon_filter_compare)) {
		g_free(value);
		g_mutex_unlock(lock);
		return FALSE;
	}

	list = g_slist_append(list, value);
	g_hash_table_insert(registry, key, list);

	g_regex_unref(regex);
	g_mutex_unlock(lock);
	return TRUE;
}

gboolean falcon_filter_unregister(gboolean is_dir, const gchar *pattern,
                                  falcon_filter_func func)
{
	GRegex *regex = NULL;
	GRegex *key = NULL;
	GSList *list = NULL;
	GSList *target = NULL;
	falcon_filter_t filter = {is_dir, func};

	if (!pattern) {
		g_warning(_("Failed to unregister filter with no pattern."));
		return FALSE;
	}

	regex = falcon_filter_regex_new(pattern);
	if (!regex)
		return FALSE;

	g_mutex_lock(lock);
	if (!registry) {
		g_critical(_("Filter registry uninitialized."
		             " Failed to unregister filter with pattern %s"), pattern);
		g_mutex_unlock(lock);
		return FALSE;
	}

	if (!g_hash_table_lookup_extended(registry, regex, (gpointer *)&key,
	                                  (gpointer *)&list)) {
		g_mutex_unlock(lock);
		return FALSE;
	}

	target = g_slist_find_custom(list, &filter, falcon_filter_compare);
	if (target) {
		g_free(target->data);
		list = g_slist_delete_link(list, target);
	}

	if (list == NULL)
		g_hash_table_remove(registry, key);
	else
		g_hash_table_insert(registry, key, list);

	g_regex_unref(regex);
	g_mutex_unlock(lock);
	return TRUE;
}

void falcon_filter_init(void)
{
	g_return_if_fail(!lock);
	g_return_if_fail(!registry);

	lock = g_mutex_new();
	registry = g_hash_table_new_full(falcon_filter_regex_hash,
	                                 falcon_filter_regex_equal,
	                                 (GDestroyNotify)g_regex_unref, NULL);
}

void falcon_filter_shutdown(void)
{
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

gboolean falcon_filter(falcon_object_t *object)
{
	GHashTableIter iter;
	GRegex *regex = NULL;
	GSList *list = NULL;
	falcon_filter_t *value = NULL;
	const gchar *name = falcon_object_get_name(object);

	g_message(_("Running filters on \"%s\"."), name);

	g_mutex_lock(lock);
	/* Call user filters. */
	if (!registry) {
		g_critical(_("Filter registry uninitialized. Failed to filter \"%s\""),
		           name);
		g_mutex_unlock(lock);
		return FALSE;
	}

	g_hash_table_iter_init(&iter, registry);
	while (g_hash_table_iter_next(&iter, (gpointer *)&regex,
	                              (gpointer *)&list)) {
		if (!g_regex_match(regex, name, 0, NULL))
			continue;

		for (; list; list = g_slist_next(list)) {
			value = (falcon_filter_t *)list->data;
			if (falcon_object_isdir(object) == value->is_dir) {
				if (!value->func || value->func(object)) {
					g_mutex_unlock(lock);
					return TRUE;
				}
			}
		}
	}

	g_mutex_unlock(lock);
	return FALSE;
}
