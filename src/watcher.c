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

#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "watcher.h"
#include "events.h"
#include "object.h"
#include "handler.h"
#include "common.h"

typedef struct {
	GMutex *lock;
	GHashTable *monitors;
	falcon_cache_t *cache;
} falcon_watcher_context_t;

static falcon_watcher_context_t context;

static void falcon_watcher_cancel(gpointer data) {
	GFileMonitor *monitor = (GFileMonitor *)data;

	g_file_monitor_cancel(monitor);
	g_object_unref(monitor);
}

static void falcon_watcher_event(GFileMonitor *monitor ATTRIBUTE_UNUSED,
                                 GFile *entity ATTRIBUTE_UNUSED,
                                 GFile *other ATTRIBUTE_UNUSED,
                                 GFileMonitorEvent event,
                                 gpointer userdata) {
	gchar *base_path = NULL;
	GFile *relative_base = NULL;
	gchar *relative_path = NULL;
	falcon_event_code_t falcon_event = EVENT_NONE;
	GFileType type = g_file_query_file_type(entity, G_FILE_QUERY_INFO_NONE,
	                                        NULL);
	falcon_object_t *object = NULL;
	struct stat info;
	memset(&info, 0, sizeof(struct stat));

	base_path = g_path_get_dirname((gchar *)userdata);
	relative_base = g_file_new_for_path(base_path);
	relative_path = g_file_get_relative_path(relative_base, entity);
	g_return_if_fail(relative_path);
	object = falcon_object_new(relative_path);
	g_free(base_path);
	g_free(relative_path);
	g_object_unref(relative_base);

	if (event != G_FILE_MONITOR_EVENT_DELETED
	    && g_stat(object->name, &info) != 0) {
		g_warning(_("Failed to obtain information for object %s, skipping..."),
		          object->name);
		falcon_object_free(object);
		return;
	}

	falcon_object_set_mode(object, info.st_mode);
	falcon_object_set_size(object, info.st_size);
	if (difftime(info.st_mtime, info.st_ctime) < 0.0)
		falcon_object_set_time(object, info.st_ctime);
	else
		falcon_object_set_time(object, info.st_mtime);
	falcon_object_set_watch(object, TRUE);

	switch (event) {
	case G_FILE_MONITOR_EVENT_CREATED:
		if (type == G_FILE_TYPE_DIRECTORY)
			falcon_event = EVENT_DIR_CREATED;
		else
			falcon_event = EVENT_FILE_CREATED;
		break;
	case G_FILE_MONITOR_EVENT_DELETED:
		if (type == G_FILE_TYPE_DIRECTORY)
			falcon_event = EVENT_DIR_DELETED;
		else
			falcon_event = EVENT_FILE_DELETED;
		break;
	case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
	case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
		if (type == G_FILE_TYPE_DIRECTORY)
			falcon_event = EVENT_DIR_CHANGED;
		else
			falcon_event = EVENT_FILE_CHANGED;
		break;
	default:
		break;
	}

	if (falcon_event != EVENT_NONE)
		falcon_handler(object, falcon_event, context.cache);

	falcon_object_free(object);
}

void falcon_watcher_init(falcon_cache_t *cache) {
	g_return_if_fail(!context.lock);
	g_return_if_fail(!context.monitors);
	g_return_if_fail(!context.cache);

	g_type_init();

	context.lock = g_mutex_new();
	context.monitors = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                         g_free, falcon_watcher_cancel);
	context.cache = cache;
}

void falcon_watcher_shutdown(void) {
	g_return_if_fail(context.lock);
	g_return_if_fail(context.monitors);

	g_mutex_free(context.lock);
	g_hash_table_unref(context.monitors);
}

gboolean falcon_watcher_add(const gchar *name) {
	gchar *filename = NULL;
	GFile *file = NULL;
	GFileMonitor *monitor = NULL;
	GError *error = NULL;

	g_return_val_if_fail(name, FALSE);

	if (!g_file_test(name, G_FILE_TEST_IS_DIR))
		return FALSE;

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to add %s, watcher not initialized yet."), name);
		return FALSE;
	}

	file = g_file_new_for_path(name);
	monitor = g_file_monitor(file, G_FILE_MONITOR_NONE, NULL, &error);
	g_object_unref(file);
	if (!monitor) {
		error->code = FALCON_ERROR_CRITICAL;
		falcon_error_report(error);
		g_error_free(error);
		return FALSE;
	}

	filename = g_strdup(name);
	g_signal_connect(monitor, "changed", (gpointer) falcon_watcher_event,
	                 filename);

	g_mutex_lock(context.lock);
	g_hash_table_insert(context.monitors, filename, monitor);
	g_mutex_unlock(context.lock);

	g_debug(_("Started watching object %s."), name);

	return TRUE;
}

gboolean falcon_watcher_delete(const gchar *name) {
	gboolean ret = FALSE;

	g_return_val_if_fail(name, FALSE);

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to delete %s, watcher not initialized yet."), name);
		return FALSE;
	}

	g_mutex_lock(context.lock);
	ret = g_hash_table_remove(context.monitors, name);
	g_mutex_unlock(context.lock);

	g_debug(_("Stopped watching object %s."), name);

	return ret;
}
