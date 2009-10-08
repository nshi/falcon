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
#include <unistd.h>
#include <errno.h>
#include <glib.h>

#include "object.h"

struct falcon_object_st {
	gchar *name;
	guint64 size;
	guint64 time;
	guint32 mode;
	gboolean watch;
};

falcon_object_t *falcon_object_new(const gchar *name)
{
	falcon_object_t *object = g_new0(falcon_object_t, 1);
	object->name = g_strdup(name);
	return object;
}

void falcon_object_free(falcon_object_t *object)
{
	g_return_if_fail(object);
	g_free(object->name);
	g_free(object);
}

falcon_object_t *falcon_object_copy(const falcon_object_t *object)
{
	falcon_object_t *ret = NULL;

	g_return_val_if_fail(object, NULL);

	ret = falcon_object_new(object->name);
	ret->mode = object->mode;
	ret->size = object->size;
	ret->time = object->time;
	ret->watch = object->watch;

	return ret;
}

void falcon_object_save(trie_node_t *node, void *userdata)
{
	const falcon_object_t *object = (const falcon_object_t *)trie_data(node);
	int fd = GPOINTER_TO_INT(userdata);
	guint16 len = strlen(object->name);
	guint16 len_be = GUINT16_TO_BE(len);
	guint64 size = GUINT64_TO_BE(object->size);
	guint64 time = GUINT64_TO_BE(object->time);
	guint32 mode = GUINT32_TO_BE(object->mode);

	if (write(fd, &len_be, 2) == -1 || write(fd, object->name, len) == -1
	    || write(fd, &size, 8) == -1 || write(fd, &time, 8) == -1
	    || write(fd, &mode, 4) == -1 || write(fd, &(object->watch), 1) == -1)
		g_critical(_("Failed to save object \"%s\": %s"), object->name,
		           g_strerror(errno));
}

gboolean falcon_object_load(falcon_object_t *object, void *userdata)
{
	int fd = GPOINTER_TO_INT(userdata);
	guint16 len = 0;

	g_return_val_if_fail(object, FALSE);

	if (read(fd, &len, 2) == -1) {
		g_critical(_("Failed to read the length of object: %s"),
		           g_strerror(errno));
		return FALSE;
	}
	len = GUINT16_FROM_BE(len);

	object->name = g_new0(gchar, len + 1);
	if (read(fd, object->name, len) == -1) {
		g_critical(_("Failed to read object name: %s"), g_strerror(errno));
		g_free(object->name);
		return FALSE;
	}

	if (read(fd, &(object->size), 8) == -1) {
		g_critical(_("Failed to read object size \"%s\": %s"), object->name,
		           g_strerror(errno));
		g_free(object->name);
		return FALSE;
	}
	object->size = GUINT64_FROM_BE(object->size);

	if (read(fd, &(object->time), 8) == -1) {
		g_critical(_("Failed to read object time \"%s\": %s"), object->name,
		           g_strerror(errno));
		g_free(object->name);
		return FALSE;
	}
	object->time = GUINT64_FROM_BE(object->time);

	if (read(fd, &(object->mode), 4) == -1) {
		g_critical(_("Failed to read object mode \"%s\": %s"), object->name,
		           g_strerror(errno));
		g_free(object->name);
		return FALSE;
	}
	object->mode = GUINT32_FROM_BE(object->mode);

	if (read(fd, &(object->watch), 1) == -1) {
		g_critical(_("Failed to read object watch \"%s\": %s"), object->name,
		           g_strerror(errno));
		g_free(object->name);
		return FALSE;
	}

	return TRUE;
}

gboolean falcon_object_equal(const falcon_object_t *a,
                             const falcon_object_t *b) {
	g_return_val_if_fail(a, FALSE);
	g_return_val_if_fail(b, FALSE);

	return (a->mode == b->mode
	        && a->size == b->size
	        && a->time == b->time
	        && g_strcmp0(a->name, b->name) == 0);
}

const gchar *falcon_object_get_name(const falcon_object_t *object)
{
	g_return_val_if_fail(object, NULL);

	return object->name;
}

gboolean falcon_object_isdir(const falcon_object_t *object)
{
	g_return_val_if_fail(object, FALSE);

	return S_ISDIR(object->mode);
}

void falcon_object_set_mode(falcon_object_t *object, mode_t mode)
{
	g_return_if_fail(object);

	object->mode = mode;
}

guint64 falcon_object_get_size(const falcon_object_t *object)
{
	g_return_val_if_fail(object, 0);

	return object->size;
}

void falcon_object_set_size(falcon_object_t *object, guint64 size)
{
	g_return_if_fail(object);

	object->size = size;
}

guint64 falcon_object_get_time(const falcon_object_t *object)
{
	g_return_val_if_fail(object, 0);

	return object->time;
}

void falcon_object_set_time(falcon_object_t *object, guint64 time)
{
	g_return_if_fail(object);

	object->time = time;
}

gboolean falcon_object_get_watch(const falcon_object_t *object)
{
	g_return_val_if_fail(object, FALSE);

	return object->watch;
}

void falcon_object_set_watch(falcon_object_t *object, gboolean watch)
{
	g_return_if_fail(object);

	object->watch = watch;
}
