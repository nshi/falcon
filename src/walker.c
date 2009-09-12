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
#include "event.h"
#include "falcon.h"

void falcon_walker_report(GError *error) {
	if (error) {
		switch (error->code) {
		case FALCON_ERROR_ERROR:
			g_error("%s: %s", g_quark_to_string(error->domain),
			        error->message);
			break;
		case FALCON_ERROR_CRITICAL:
			g_critical("%s: %s", g_quark_to_string(error->domain),
			           error->message);
			break;
		case FALCON_ERROR_WARNING:
			g_warning("%s: %s", g_quark_to_string(error->domain),
			          error->message);
			break;
		default:
			g_message("%s: %s", g_quark_to_string(error->domain),
			          error->message);
		}
	}
}

void falcon_walker_walk_dir(const gchar *name, falcon_cache_t *cache) {
	GDir *dir = NULL;
	GError *error = NULL;
	const gchar *entry = NULL;
	gchar *path = NULL;
	falcon_object_t *object = NULL;

	g_return_if_fail(name);
	g_return_if_fail(cache);

	dir = g_dir_open(name, 0, &error);
	if (!dir) {
		falcon_walker_report(error);
		return;
	}

	while ((entry = g_dir_read_name(dir))) {
		/* Should detect OS when building path. */
		path = g_build_path("/", name, entry);

		object = falcon_cache_get_object(cache, path);
		if (!object)
			object = falcon_object_new(path);
		falcon_task_add(object);

		g_free(path);
		path = NULL;
	}

	g_dir_close(dir);
}

gboolean falcon_walker_runeach(falcon_object_t *object, falcon_cache_t *cache) {
	falcon_object_t *cached = NULL;
	falcon_event_code_t event = EVENT_NONE;
	struct stat info;
	memset(&info, 0, sizeof(struct stat));

	g_return_val_if_fail(object, -1);

	if (!(object->name))
		g_warning(_("Object has no path associated with it, skipping..."));

	cached = falcon_cache_get_object(cache, object->name);

	if (cache && !g_file_test(object->name, G_FILE_TEST_EXISTS)) {
		if (S_ISDIR(cached->mode))
			falcon_handler(object, EVENT_DIR_DELETED, cache);
		else
			falcon_handler(object, EVENT_FILE_DELETED, cache);
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
			falcon_walker_walk_dir(object->name, cache);
		} else {
			if (!falcon_object_compare(object, cache)) {
				event = EVENT_DIR_CHANGED;
				falcon_walker_walk_dir(object->name, cache);
			}
		}
	} else if (g_file_test(object->name, G_FILE_TEST_IS_REGULAR)) {
		/* Handle file. */
		if (!cached)
			event = EVENT_FILE_CREATED;
		else {
			if (!falcon_object_compare(object, cache))
				event = EVENT_FILE_CHANGED;
		}
	}

	if (event != EVENT_NONE)
		falcon_handler(object, event, cache);

	return TRUE;
}

void falcon_walker_run(gpointer data, gpointer userdata) {
	GQueue *objects = (GQueue *)data;
	falcon_object_t *object = NULL;
	falcon_cache_t *cache = (falcon_cache_t *)userdata;
	guint i;
	GError *error = NULL;

	g_return_if_fail(objects);
	g_return_if_fail(cache);

	if (!cache) {
		g_set_error(&error, FALCON_WALKER_ERROR, FALCON_ERROR_CRITICAL,
		            _("Cache not provided. Walker thread returning."));
		goto finish;
	}

	while (!g_queue_is_empty(objects)) {
		object = g_queue_pop_head(objects);
		if (!falcon_walker_runeach(object, cache)) {
			falcon_failed_add(object);
		}
	}

finish:
	if (failed_objects != objects)
		g_queue_free(objects);
	falcon_walker_report(failed_objects, error);
	if (!error)
		g_error_free(error);
}