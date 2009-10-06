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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "object.h"

gboolean cache_load(const gchar *name)
{
	int fd = 0;
	guint64 count = 0;
	guint64 i;
	falcon_object_t *object = NULL;
	gboolean ret = TRUE;

	if (!name)
		return FALSE;

	fd = g_open(name, O_RDONLY, 0);
	if (fd == -1) {
		g_critical(_("Failed to read cache file %s: %s"), name,
		           g_strerror(errno));
		return FALSE;
	}

	if (read(fd, &count, 8) == -1) {
		g_critical(_("Failed to read cache file %s: %s"), name,
		           g_strerror(errno));
		close(fd);
		return FALSE;
	}
	count = GUINT64_FROM_BE(count);

	g_debug(_("Loaded %lu cache keys."), count);

	for (i = 1; i <= count; i++) {
		object = falcon_object_new(NULL);
		if (!falcon_object_load(object, GINT_TO_POINTER(fd))) {
			g_critical(_("Failed to load object %lu"), i);
			falcon_object_free(object);
			ret = FALSE;
			break;
		}

		g_debug(_("\"%s\": dir=%s, size=%ld, time=%ld, watch=%s"),
		        falcon_object_get_name(object),
		        falcon_object_isdir(object) ? "yes" : "no",
		        falcon_object_get_size(object),
		        falcon_object_get_time(object),
		        falcon_object_get_watch(object) ? "yes" : "no");

		falcon_object_free(object);
	}

	close(fd);

	return ret;
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return -1;

	setlocale(LC_ALL, "");

	cache_load(argv[1]);

	return 0;
}
