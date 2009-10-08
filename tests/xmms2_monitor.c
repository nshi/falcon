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

#include <stdlib.h>
#include <glib.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

#include "falcon.h"
#include "handler.h"
#include "object.h"
#include "events.h"
#include "common.h"

typedef struct {
	xmmsc_connection_t *conn;
	GMainLoop *ml;
	gchar *cache_name;
} monitor_t;

static gboolean
event_handler(falcon_object_t *object, falcon_event_code_t event,
              gpointer userdata);

static int
monitor_falcon_config_get(xmmsv_t *value, void *udata)
{
	monitor_t *monitor = (monitor_t *)udata;
	const gchar *path = NULL;

	if (!xmmsv_get_string(value, &path)) {
		g_error(_("Failed to retrieve cache file path."));
		return FALSE;
	}

	if (strlen(path) > 0)
		monitor->cache_name = g_strdup(path);

	falcon_set_log_level(G_LOG_LEVEL_MESSAGE);
	falcon_init(monitor->cache_name);
	falcon_handler_register(EVENT_FILE_ALL, event_handler, monitor->conn);

	return FALSE;
}

static int
monitor_falcon_init(xmmsv_t *value, void *udata)
{
	monitor_t *monitor = (monitor_t *)udata;
	xmmsc_result_t *res = NULL;
	const gchar *conf = NULL;

	if (!xmmsv_get_string(value, &conf)) {
		g_error(_("Failed to get falcon config."));
		return FALSE;
	}

	res = xmmsc_config_get_value(monitor->conn, conf);
	xmmsc_result_notifier_set(res, monitor_falcon_config_get, monitor);
	xmmsc_result_unref(res);

	return FALSE;
}

static void
monitor_init(monitor_t *monitor)
{
	monitor->conn = xmmsc_init("XMMS2-Medialib-Monitor");
}

static void
monitor_shutdown(xmmsc_connection_t *conn)
{
	g_return_if_fail(conn);

	xmmsc_unref(conn);
}

static void
monitor_quit(void *data)
{
	monitor_t *monitor = (monitor_t *)data;

	g_debug(_("Shutting down the monitor."));

	monitor_shutdown(monitor->conn);
	g_main_loop_quit(monitor->ml);
	falcon_shutdown(monitor->cache_name, TRUE);
}

static gboolean
monitor_connect(monitor_t *monitor)
{
	gchar *path = NULL;

	g_return_val_if_fail(monitor, FALSE);
	g_return_val_if_fail(monitor->conn, FALSE);

	path = getenv("XMMS_PATH");

	if (!xmmsc_connect(monitor->conn, path)) {
		g_warning(_("Unable to connect to XMMS2."));
		return FALSE;
	}

	xmmsc_mainloop_gmain_init(monitor->conn);
	xmmsc_disconnect_callback_set(monitor->conn, monitor_quit, monitor);

	return TRUE;
}

static gboolean
monitor_switch_dir(const gchar *path)
{
	g_return_val_if_fail(path, FALSE);

	if (!falcon_has(path)) {
		falcon_clear();
		falcon_add(path, TRUE);
	}

	return TRUE;
}

static int
monitor_config_changed(xmmsv_t *value, void *udata ATTRIBUTE_UNUSED)
{
	const gchar *path = NULL;

	if (xmmsv_dict_entry_get_string(value, "clients.mlibmonitor.watch_dirs",
	                                &path)) {
		if (strlen(path) > 0)
			monitor_switch_dir(path);
	}

	return TRUE;
}

static int
monitor_config_get(xmmsv_t *value, void *udata ATTRIBUTE_UNUSED)
{
	const gchar *path = NULL;

	if (!xmmsv_get_string(value, &path)) {
		g_error(_("Failed to retrieve config value."));
		return FALSE;
	}

	if (strlen(path) > 0)
		monitor_switch_dir(path);

	return FALSE;
}

static int
monitor_config_register(xmmsv_t *value, void *udata)
{
	monitor_t *monitor = (monitor_t *)udata;
	xmmsc_result_t *res = NULL;
	const gchar *conf = NULL;

	if (!xmmsv_get_string(value, &conf)) {
		g_error(_("Failed to register config value."));
		return FALSE;
	}

	res = xmmsc_config_get_value(monitor->conn, conf);
	xmmsc_result_notifier_set(res, monitor_config_get, NULL);
	xmmsc_result_unref(res);

	return FALSE;
}

static void
monitor_subscribe_config(monitor_t *monitor)
{
	xmmsc_result_t *res;

	g_return_if_fail(monitor);
	g_return_if_fail(monitor->conn);

	res = xmmsc_config_register_value(monitor->conn,
	                                  "mlibmonitor.cache_path", "");
	xmmsc_result_notifier_set(res, monitor_falcon_init, monitor);
	xmmsc_result_unref(res);

	res = xmmsc_config_register_value(monitor->conn,
	                                  "mlibmonitor.watch_dirs", "");
	xmmsc_result_notifier_set(res, monitor_config_register, monitor);
	xmmsc_result_unref(res);

	res = xmmsc_broadcast_config_value_changed(monitor->conn);
	xmmsc_result_notifier_set(res, monitor_config_changed, monitor);
	xmmsc_result_unref(res);
}

static int
monitor_file_changed(xmmsv_t *value, void *udata ATTRIBUTE_UNUSED)
{
	const char *err = NULL;

	if (xmmsv_is_error(value)) {
		xmmsv_get_error(value, &err);
		g_warning(_("Error: %s"), err);
		return FALSE;
	}

	return TRUE;
}

static void
monitor_add_file(xmmsc_connection_t *conn, const falcon_object_t *file)
{
	xmmsc_result_t *res = NULL;
	const gchar *path = NULL;
	gchar *url = NULL;

	g_return_if_fail(conn);
	g_return_if_fail(file);

	path = falcon_object_get_name(file);
	url = g_strdup_printf("file://%s", path);

	res = xmmsc_medialib_add_entry(conn, url);
	g_free(url);
	xmmsc_result_notifier_set(res, monitor_file_changed, NULL);
	xmmsc_result_unref(res);
}

static int
monitor_remove_file_by_id(xmmsv_t *value, void *udata)
{
	xmmsc_connection_t *conn = (xmmsc_connection_t *)udata;
	xmmsc_result_t *res;
	int mid;

	if (!xmmsv_get_int(value, &mid)) {
		g_error(_("Couldn't find this one!"));
		return FALSE;
	}

	if (!mid) {
		g_warning(_("Entry not in medialib"));
		return FALSE;
	}

	res = xmmsc_medialib_remove_entry(conn, mid);
	xmmsc_result_notifier_set(res, monitor_file_changed, NULL);
	xmmsc_result_unref(res);

	return FALSE;
}

static void
monitor_remove_file(xmmsc_connection_t *conn, const falcon_object_t *file)
{
	xmmsc_result_t *res = NULL;
	const gchar *path = NULL;
	gchar *url = NULL;

	g_return_if_fail(conn);
	g_return_if_fail(file);

	path = falcon_object_get_name(file);
	url = g_strdup_printf("file://%s", path);

	res = xmmsc_medialib_get_id(conn, url);
	xmmsc_result_notifier_set(res, monitor_remove_file_by_id, conn);
	xmmsc_result_unref(res);

	g_free(url);
}

static int
monitor_rehash_file_by_id(xmmsv_t *value, void *udata)
{
	xmmsc_connection_t *conn = (xmmsc_connection_t *)udata;
	xmmsc_result_t *res;
	int mid;

	if (!xmmsv_get_int(value, &mid))
		return FALSE;

	if (!mid) {
		g_warning(_("Couldn't find requested medialib entry."));
		return FALSE;
	}

	res = xmmsc_medialib_rehash(conn, mid);
	xmmsc_result_notifier_set(res, monitor_file_changed, NULL);
	xmmsc_result_unref(res);

	return FALSE;
}

static void
monitor_rehash_file(xmmsc_connection_t *conn, const falcon_object_t *file)
{
	xmmsc_result_t *res = NULL;
	const gchar *path = NULL;
	gchar *url = NULL;

	g_return_if_fail(conn);
	g_return_if_fail(file);

	path = falcon_object_get_name(file);
	url = g_strdup_printf("file://%s", path);

	g_debug(_("Resolving entry '%s'"), url);

	res = xmmsc_medialib_get_id(conn, url);
	xmmsc_result_notifier_set(res, monitor_rehash_file_by_id, conn);
	xmmsc_result_unref(res);

	g_free(url);
}

static void
on_entity_created(xmmsc_connection_t *conn, const falcon_object_t *entity)
{
	g_return_if_fail(conn);
	g_return_if_fail(entity);

	monitor_add_file(conn, entity);
}

static void
on_entity_changed(xmmsc_connection_t *conn, const falcon_object_t *entity)
{
	g_return_if_fail(conn);
	g_return_if_fail(entity);

	monitor_rehash_file(conn, entity);
}

static void
on_entity_deleted(xmmsc_connection_t *conn, const falcon_object_t *entity)
{
	g_return_if_fail(conn);
	g_return_if_fail(entity);

	monitor_remove_file(conn, entity);
}

static gboolean
event_handler(falcon_object_t *object, falcon_event_code_t event,
              gpointer userdata) {
	xmmsc_connection_t *conn = (xmmsc_connection_t *)userdata;

	switch (event) {
	case EVENT_FILE_CREATED:
		on_entity_created(conn, object);
		break;
	case EVENT_FILE_CHANGED:
		on_entity_changed(conn, object);
		break;
	case EVENT_FILE_DELETED:
		on_entity_deleted(conn, object);
		break;
	default:
		break;
	}

	return TRUE;
}

int
main(void)
{
	monitor_t *monitor = g_new0(monitor_t, 1);

	g_thread_init(NULL);
	monitor_init(monitor);

	monitor->ml = g_main_loop_new(NULL, FALSE);

	if (!monitor_connect(monitor)) {
		monitor_shutdown(monitor->conn);
		g_main_loop_unref(monitor->ml);
		return -1;
	}

	monitor_subscribe_config(monitor);

	g_main_loop_run(monitor->ml);
	g_main_loop_unref(monitor->ml);

	g_free(monitor->cache_name);
	g_free(monitor);

	return 0;
}
