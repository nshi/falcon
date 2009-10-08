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

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <sys/types.h>
#include <glib.h>

#include "common.h"
#include "trie.h"

/*
 * Only one thread will modify a single object at a time, so there's no need to
 * lock it.
 */
typedef struct falcon_object_st falcon_object_t;

/*
 * If name is not NULL, it must be a NULL-terminated string.
 */
falcon_object_t *falcon_object_new(const gchar *name);
void falcon_object_free(falcon_object_t *object);
falcon_object_t *falcon_object_copy(const falcon_object_t *object);
/*
 * Saves a single object to a file based on the binary file format.
 */
void falcon_object_save(trie_node_t *node, void *userdata);
/*
 * Loads a single object from a file based on the binary file format.
 *
 * The object has to be created before calling this function.
 */
gboolean falcon_object_load(falcon_object_t *object, void *userdata);

/*
 * This doesn't compare the watchability field, because it's not considered as
 * an attribute of the actual object on the file system.
 */
inline gboolean falcon_object_equal(const falcon_object_t *a,
                                    const falcon_object_t *b);
inline const gchar *falcon_object_get_name(const falcon_object_t *object);
inline gboolean falcon_object_isdir(const falcon_object_t *object);
inline void falcon_object_set_mode(falcon_object_t *object, mode_t mode);
inline guint64 falcon_object_get_size(const falcon_object_t *object);
inline void falcon_object_set_size(falcon_object_t *object, guint64 size);
inline guint64 falcon_object_get_time(const falcon_object_t *object);
inline void falcon_object_set_time(falcon_object_t *object, guint64 time);
inline gboolean falcon_object_get_watch(const falcon_object_t *object);
inline void falcon_object_set_watch(falcon_object_t *object, gboolean watch);

#endif
