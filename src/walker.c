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
#include <glib/gstdio.h>

#include "walker.h"
#include "event.h"

gboolean falcon_walker_runeach(falcon_object_t *object,
                               falcon_walker_userdata_t *user) {
	g_return_val_if_fail(object, -1);

	if (!(object->name))
		g_warning(_("Object has no path associated with it, skipping..."));

	if (g_file_test(object->name, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		/* Handle directory. */

	} else if (g_file_test(object->name,
	                       G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
		/* Handle file. */

	}

	return TRUE;
}

void falcon_walker_run(gpointer data, gpointer userdata) {
	GQueue *objects = (GQueue *)data;
	GQueue *failed_objects = objects;
	falcon_object_t *object = NULL;
	falcon_walker_userdata_t *user = (falcon_walker_userdata_t *)userdata;
	guint i;
	GError *error = NULL;

	g_return_if_fail(objects);
	g_return_if_fail(user);

	if (!(user->falcon_walker_return)) {
		g_critical(_("Walker return function not provided."
		             " Walker thread terminating."));
		return;
	}
	if (!(user->cache)) {
		g_set_error(&error, FALCON_WALKER_ERROR, FALCON_ERROR_CRITICAL,
		            _("Cache not provided. Walker thread returning."));
		goto finish;
	}
	if (!(user->queue)) {
		g_set_error(&error, FALCON_WALKER_ERROR, FALCON_ERROR_CRITICAL,
		            _("Object queue not provided. Walker thread returning."));
		goto finish;
	}

	failed_objects = g_queue_new();

	while (!g_queue_is_empty(objects)) {
		object = g_queue_pop_head(objects);
		if (falcon_walker_runeach(object, user)) {
			falcon_object_free(object);
		} else {
			g_queue_push_tail(failed_objects, object);
		}
	}

finish:
	if (failed_objects != objects)
		g_queue_free(objects);
	user->falcon_walker_return(failed_objects, error);
	if (!error)
		g_error_free(error);
}
