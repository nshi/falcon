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

#ifndef _COMMON_H_
#define _COMMON_H_

#include <glib.h>

#define GETTEXT_PACKAGE "falcon01"
#include <glib/gi18n-lib.h>

#include "object.h"

#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__((__unused__))
#endif

/* Errors */
#define FALCON_ERROR_ERROR G_LOG_LEVEL_ERROR
#define FALCON_ERROR_CRITICAL G_LOG_LEVEL_CRITICAL
#define FALCON_ERROR_WARNING G_LOG_LEVEL_WARNING

/* Constants */
#define MAX_WALKERS 3
#define OBJECTS_PER_THREAD 20

void falcon_log_handler (const gchar *log_domain, GLogLevelFlags log_level,
                         const gchar *message, gpointer user_data);
void falcon_set_log_level(GLogLevelFlags log_level);

gint falcon_object_compare(gconstpointer a, gconstpointer b);
void falcon_error_report(GError *error);

#endif
