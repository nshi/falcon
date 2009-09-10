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

#ifndef _CACHE_H_
#define _CACHE_H_

#include <glib.h>

#include "common.h"

typedef struct {
	GMutex *lock;
	GQueue *objects;
} falcon_cache_t;

falcon_cache_t *falcon_cache_new(void);
void falcon_cache_free(falcon_cache_t *cache);

/*
 * Gets the object with the given name. If the object is not found, return NULL.
 */
falcon_object_t *falcon_cache_get_object(falcon_cache_t *cache,
                                         const gchar *name);
/*
 * Adds an object to the cache. If another object with the same name exists, the
 * old one is updated with the new one.
 */
gboolean falcon_cache_add_object(falcon_cache_t *cache,
                                 falcon_object_t *object);
gboolean falcon_cache_delete_object(falcon_cache_t *cache,
                                    falcon_object_t *object);

#endif
