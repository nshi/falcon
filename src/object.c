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
#include <sys/stat.h>

#include "object.h"

falcon_object_t *falcon_object_new(const gchar *name) {
	g_return_val_if_fail(name, NULL);

	falcon_object_t *object = g_new0(falcon_object_t, 1);
	object->name = g_strdup(name);
	return object;
}

void falcon_object_free(falcon_object_t *object) {
	g_return_if_fail(object);
	g_free(object->name);
	g_free(object);
}

gboolean falcon_object_compare(const falcon_object_t *a,
                               const falcon_object_t *b) {
	g_return_val_if_fail(a, FALSE);
	g_return_val_if_fail(b, FALSE);

	return (a->mode == b->mode
	        && a->size == b->size
	        && a->time == b->time
	        && g_strcmp0(a->name, b->name));
}

gboolean falcon_object_isdir(const falcon_object_t *object) {
	g_return_val_if_fail(object, FALSE);

	return S_ISDIR(object->mode);
}

void falcon_object_set_mode(falcon_object_t *object, mode_t mode) {
	g_return_if_fail(object);

	object->mode = mode;
}

off_t falcon_object_get_size(const falcon_object_t *object) {
	g_return_val_if_fail(object, 0);

	return object->size;
}

void falcon_object_set_size(falcon_object_t *object, off_t size) {
	g_return_if_fail(object);

	object->size = size;
}

time_t falcon_object_get_time(const falcon_object_t *object) {
	g_return_val_if_fail(object, 0);

	return object->time;
}

void falcon_object_set_time(falcon_object_t *object, time_t time) {
	g_return_if_fail(object);

	object->time = time;
}

gboolean falcon_object_get_watch(const falcon_object_t *object) {
	g_return_val_if_fail(object, FALSE);

	return object->watch;
}

void falcon_object_set_watch(falcon_object_t *object, gboolean watch) {
	g_return_if_fail(object);

	object->watch = watch;
}
