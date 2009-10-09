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
	gchar *cwd;
} falcon_watcher_context_t;

static falcon_watcher_context_t context;

static void falcon_watcher_cancel(gpointer data)
{
	GFileMonitor *monitor = (GFileMonitor *)data;

	g_file_monitor_cancel(monitor);
	g_object_unref(monitor);
}

static void falcon_watcher_event(GFileMonitor *monitor ATTRIBUTE_UNUSED,
                                 GFile *entity,
                                 GFile *other ATTRIBUTE_UNUSED,
                                 GFileMonitorEvent event ATTRIBUTE_UNUSED,
                                 gpointer userdata)
{
	gboolean absolute = GPOINTER_TO_INT(userdata);
	GFile *relative_base = NULL;
	gchar *path = NULL;
	falcon_object_t *object = NULL;

	if (absolute) {
		path = g_file_get_path(entity);
	} else {
		relative_base = g_file_new_for_path(context.cwd);
		path = g_file_get_relative_path(relative_base, entity);
		g_return_if_fail(path);
		g_object_unref(relative_base);
	}
	object = falcon_object_new(path);
	falcon_object_set_watch(object, TRUE);
	g_free(path);

	falcon_task_add(object);
}

void falcon_watcher_init(falcon_cache_t *cache)
{
	g_return_if_fail(!context.lock);
	g_return_if_fail(!context.monitors);
	g_return_if_fail(!context.cache);
	g_return_if_fail(!context.cwd);

	g_type_init();

	context.lock = g_mutex_new();
	context.monitors = g_hash_table_new_full(g_str_hash, g_str_equal,
	                                         g_free, falcon_watcher_cancel);
	context.cache = cache;
	context.cwd = g_get_current_dir();
}

void falcon_watcher_shutdown(void)
{
	g_return_if_fail(context.lock);
	g_return_if_fail(context.monitors);
	g_return_if_fail(context.cwd);

	g_mutex_free(context.lock);
	g_hash_table_unref(context.monitors);
	g_free(context.cwd);
}

gboolean falcon_watcher_add(const falcon_object_t *object)
{
	GFile *file = NULL;
	GFileMonitor *monitor = NULL;
	GError *error = NULL;
	gboolean absolute = FALSE;

	g_return_val_if_fail(object, FALSE);
	g_return_val_if_fail(falcon_object_isdir(object), FALSE);

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to add %s, watcher not initialized yet."),
		           falcon_object_get_name(object));
		return FALSE;
	}

	g_mutex_lock(context.lock);
	if (g_hash_table_lookup(context.monitors, falcon_object_get_name(object))) {
		g_mutex_unlock(context.lock);
		return FALSE;
	}
	g_mutex_unlock(context.lock);

	file = g_file_new_for_path(falcon_object_get_name(object));
	monitor = g_file_monitor(file, G_FILE_MONITOR_NONE, NULL, &error);
	g_object_unref(file);
	if (!monitor) {
		error->code = FALCON_ERROR_CRITICAL;
		falcon_error_report(error);
		g_error_free(error);
		return FALSE;
	}

	absolute = g_path_is_absolute(falcon_object_get_name(object));
	g_signal_connect(monitor, "changed", (gpointer) falcon_watcher_event,
	                 GINT_TO_POINTER(absolute));

	g_mutex_lock(context.lock);
	g_hash_table_insert(context.monitors,
	                    g_strdup(falcon_object_get_name(object)), monitor);
	g_mutex_unlock(context.lock);

	g_debug(_("Started watching object %s."), falcon_object_get_name(object));

	return TRUE;
}

gboolean falcon_watcher_delete(const falcon_object_t *object)
{
	gboolean ret = FALSE;

	g_return_val_if_fail(object, FALSE);

	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to delete %s, watcher not initialized yet."),
		           falcon_object_get_name(object));
		return FALSE;
	}

	g_mutex_lock(context.lock);
	ret = g_hash_table_remove(context.monitors, falcon_object_get_name(object));
	g_mutex_unlock(context.lock);

	g_debug(_("Stopped watching object %s."), falcon_object_get_name(object));

	return ret;
}

void falcon_watcher_clear(void)
{
	if (!context.monitors || !context.lock || !context.cache) {
		g_critical(_("Failed to clear watchers"));
		return;
	}

	g_mutex_lock(context.lock);
	g_hash_table_remove_all(context.monitors);
	g_mutex_unlock(context.lock);

	g_debug(_("Stopped watching all objects."));
}
