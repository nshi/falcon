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

#ifndef _FALCON_H_
#define _FALCON_H_

#include <glib.h>

/* Errors */
#define FALCON_ERROR_ERROR G_LOG_LEVEL_ERROR
#define FALCON_ERROR_CRITICAL G_LOG_LEVEL_CRITICAL
#define FALCON_ERROR_WARNING G_LOG_LEVEL_WARNING

void falcon_set_log_level(GLogLevelFlags log_level);

typedef enum {
	EVENT_NONE = 0,

	/* Directory */
	EVENT_DIR_CREATED = (1 << 0),
	EVENT_DIR_DELETED = (1 << 1),
	EVENT_DIR_CHANGED = (1 << 2),

	/* File */
	EVENT_FILE_CREATED = (1 << 3),
	EVENT_FILE_DELETED = (1 << 4),
	EVENT_FILE_CHANGED = (1 << 5),

	EVENT_DIR_ALL = (EVENT_DIR_CREATED | EVENT_DIR_DELETED | EVENT_DIR_CHANGED),
	EVENT_FILE_ALL = (EVENT_FILE_CREATED | EVENT_FILE_DELETED
	                  | EVENT_FILE_CHANGED),
	EVENT_ALL = (EVENT_DIR_ALL | EVENT_FILE_ALL)
} falcon_event_code_t;

const gchar *falcon_event_to_string(falcon_event_code_t event);

/*
 * Only one thread will modify a single object at a time, so there's no need to
 * lock it.
 */
typedef struct falcon_object_st falcon_object_t;

/*
 * This doesn't compare the watchability field, because it's not considered as
 * an attribute of the actual object on the file system.
 */
gboolean falcon_object_equal(const falcon_object_t *a, const falcon_object_t *b);
const gchar *falcon_object_get_name(const falcon_object_t *object);
gboolean falcon_object_isdir(const falcon_object_t *object);
guint64 falcon_object_get_size(const falcon_object_t *object);
guint64 falcon_object_get_time(const falcon_object_t *object);
gboolean falcon_object_get_watch(const falcon_object_t *object);

void falcon_init(const gchar *name);
/* Waits for all the tasks to be finished if wait is TRUE. */
void falcon_shutdown(const gchar *name, gboolean wait);

/*
 * Adds an object with the given name to the cache. Watch the directory for
 * changes if watch is TRUE.
 */
gboolean falcon_add(const gchar *name, gboolean watch);
/*
 * Deletes an object with the given name as well as its descendants.
 *
 * Attention: this is a blocking function. It has to wait until all the
 * currently pending objects have been processed.
 */
gboolean falcon_delete(const gchar *name);
/*
 * Clears all objects.
 *
 * Attention: this is a blocking function. It has to wait until all the
 * currently pending objects have been processed.
 */
void falcon_clear(void);
/*
 * Sets the watchability flag of an object to watch.
 */
gboolean falcon_set_watch(const gchar *name, gboolean watch);
/*
 * Checks if the given path is already in the cache.
 */
gboolean falcon_has(const gchar *name);

typedef gboolean (*falcon_handler_func)(falcon_object_t *object,
                                        falcon_event_code_t event,
                                        gpointer userdata);

gboolean falcon_handler_register(falcon_event_code_t events,
                                 falcon_handler_func func, gpointer userdata);
gboolean falcon_handler_unregister(falcon_event_code_t events,
                                   falcon_handler_func func);

typedef gboolean (*falcon_filter_func)(falcon_object_t *object);

gboolean falcon_filter_register(gboolean is_dir, const gchar *pattern,
                                falcon_filter_func func);
gboolean falcon_filter_unregister(gboolean is_dir, const gchar *pattern,
                                  falcon_filter_func func);

#endif
