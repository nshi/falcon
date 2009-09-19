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

#include <time.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "walker.h"
#include "events.h"
#include "falcon.h"
#include "handler.h"
#include "watcher.h"

static void falcon_walker_walk_dir(const falcon_object_t *parent,
                                   const falcon_object_t *cached) {
	GDir *dir = NULL;
	GError *error = NULL;
	const gchar *entry = NULL;
	gchar *path = NULL;
	falcon_object_t *object = NULL;

	g_return_if_fail(parent);

	g_debug(_("Walking directory \"%s\"."), parent->name);

	dir = g_dir_open(parent->name, 0, &error);
	if (!dir) {
		falcon_error_report(error);
		g_error_free(error);
		return;
	}

	while ((entry = g_dir_read_name(dir))) {
		path = g_build_path(G_DIR_SEPARATOR_S, parent->name, entry,
		                    (const gchar *)NULL);
		object = falcon_object_new(path);
		if (cached)
			falcon_object_set_watch(object, falcon_object_get_watch(cached));
		else
			falcon_object_set_watch(object, falcon_object_get_watch(parent));

		if (object->watch)
			falcon_watcher_add(object->name);

		falcon_task_add(object);

		g_free(path);
		path = NULL;
	}

	g_dir_close(dir);
}

static gboolean falcon_walker_runeach(falcon_object_t *object,
                                      falcon_cache_t *cache) {
	falcon_object_t *cached = NULL;
	falcon_event_code_t event = EVENT_NONE;
	struct stat info;
	memset(&info, 0, sizeof(struct stat));

	g_return_val_if_fail(object, FALSE);

	if (!(object->name))
		g_warning(_("Object has no path associated with it, skipping..."));

	cached = falcon_cache_get(cache, object->name);

	if (cached && !g_file_test(object->name, G_FILE_TEST_EXISTS)) {
		if (S_ISDIR(cached->mode))
			falcon_handler(cached, EVENT_DIR_DELETED, cache);
		else
			falcon_handler(cached, EVENT_FILE_DELETED, cache);
		falcon_object_free(object);
		return TRUE;
	}


	if (g_stat(object->name, &info) != 0) {
		g_warning(_("Failed to obtain information for object %s, skipping..."),
		          object->name);
		return FALSE;
	}

	falcon_object_set_mode(object, info.st_mode);
	falcon_object_set_size(object, info.st_size);
	if (difftime(info.st_mtime, info.st_ctime) < 0.0)
		falcon_object_set_time(object, info.st_ctime);
	else
		falcon_object_set_time(object, info.st_mtime);
	if (g_file_test(object->name, G_FILE_TEST_IS_DIR)) {
		/* Handle directory. */
		if (!cached) {
			event = EVENT_DIR_CREATED;
			falcon_walker_walk_dir(object, NULL);
		} else if (!falcon_object_equal(object, cached)) {
			event = EVENT_DIR_CHANGED;
			falcon_walker_walk_dir(object, cached);
		}
	} else if (g_file_test(object->name, G_FILE_TEST_IS_REGULAR)) {
		/* Handle file. */
		if (!cached)
			event = EVENT_FILE_CREATED;
		else if (!falcon_object_equal(object, cached))
			event = EVENT_FILE_CHANGED;
	}

	if (event != EVENT_NONE)
		falcon_handler(object, event, cache);

	falcon_object_free(object);

	return TRUE;
}

void falcon_walker_run(gpointer data, gpointer userdata) {
	GQueue *objects = (GQueue *)data;
	falcon_object_t *object = NULL;
	falcon_cache_t *cache = (falcon_cache_t *)userdata;
	GError *error = NULL;

	g_return_if_fail(objects);
	g_return_if_fail(cache);

	g_debug(_("Walker started with %d objects."), g_queue_get_length(objects));

	if (!cache) {
		g_set_error(&error, FALCON_WALKER_ERROR, FALCON_ERROR_CRITICAL,
		            _("Cache not provided. Walker thread returning."));

		g_queue_free(objects);
		falcon_walker_return(error);
		g_error_free(error);
		return;
	}

	while (!g_queue_is_empty(objects)) {
		object = g_queue_pop_head(objects);
		if (!falcon_walker_runeach(object, cache))
			falcon_failed_add(object);
	}

	g_queue_free(objects);
	falcon_walker_return(NULL);
}
