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

#ifndef _HANDLER_H_
#define _HANDLER_H_

#include "common.h"
#include "object.h"
#include "events.h"
#include "cache.h"

typedef gboolean (*falcon_handler_func)(falcon_object_t *object,
                                        falcon_event_code_t event,
                                        gpointer userdata);

gboolean falcon_handler_register(falcon_event_code_t events,
                                 falcon_handler_func func, gpointer userdata);
gboolean falcon_handler_unregister(falcon_event_code_t events,
                                   falcon_handler_func func);

/*
 * This should be called at startup.
 */
void falcon_handler_registry_init(void);
void falcon_handler_registry_shutdown(void);

/*
 * This is the default event handler. It checks if the user application has
 * registered any custom handlers. If yes, call them one by one.
 *
 * If the custom event handler returns FALSE, the handler will be unregistered.
 */
void falcon_handler(falcon_object_t *object, falcon_event_code_t event,
                    falcon_cache_t *cache);

#endif
