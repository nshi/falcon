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
#include "object.h"
#include "common.h"
#include "falcon.h"

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
                                 GFileMonitorEvent event ATTRIBUTE_UNUSED,
                                 gpointer userdata) {
	gchar *base_path = NULL;
	GFile *relative_base = NULL;
	gchar *relative_path = NULL;
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

	falcon_task_add(object);
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

gboolean falcon_watcher_add(const falcon_object_t *object) {
	gchar *filename = NULL;
	GFile *file = NULL;
	GFileMonitor *monitor = NULL;
	GError *error = NULL;

	g_return_val_if_fail(object, FALSE);
	g_return_val_if_fail(S_ISDIR(object->mode), FALSE);

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to add %s, watcher not initialized yet."),
		           object->name);
		return FALSE;
	}

	g_mutex_lock(context.lock);
	if (g_hash_table_lookup(context.monitors, object->name)) {
		g_mutex_unlock(context.lock);
		return FALSE;
	}
	g_mutex_unlock(context.lock);

	file = g_file_new_for_path(object->name);
	monitor = g_file_monitor(file, G_FILE_MONITOR_NONE, NULL, &error);
	g_object_unref(file);
	if (!monitor) {
		error->code = FALCON_ERROR_CRITICAL;
		falcon_error_report(error);
		g_error_free(error);
		return FALSE;
	}

	filename = g_strdup(object->name);
	g_signal_connect(monitor, "changed", (gpointer) falcon_watcher_event,
	                 filename);

	g_mutex_lock(context.lock);
	g_hash_table_insert(context.monitors, filename, monitor);
	g_mutex_unlock(context.lock);

	g_debug(_("Started watching object %s."), object->name);

	return TRUE;
}

gboolean falcon_watcher_delete(const falcon_object_t *object) {
	gboolean ret = FALSE;

	g_return_val_if_fail(object, FALSE);

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to delete %s, watcher not initialized yet."),
		           object->name);
		return FALSE;
	}

	g_mutex_lock(context.lock);
	ret = g_hash_table_remove(context.monitors, object->name);
	g_mutex_unlock(context.lock);

	g_debug(_("Stopped watching object %s."), object->name);

	return ret;
}
