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

#include "common.h"

#define DEFAULT_LOG_LEVEL (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR \
                           | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION)

static GLogLevelFlags falcon_log_level = DEFAULT_LOG_LEVEL;

void falcon_log_handler (const gchar *log_domain, GLogLevelFlags log_level,
                         const gchar *message, gpointer user_data) {
	if (log_level > falcon_log_level)
		return;

	g_log_default_handler(log_domain, log_level, message, user_data);
}

void falcon_set_log_level(GLogLevelFlags log_level)
{
	falcon_log_level  = DEFAULT_LOG_LEVEL;

	if (log_level >= G_LOG_LEVEL_WARNING)
		falcon_log_level |= G_LOG_LEVEL_WARNING;
	if (log_level >= G_LOG_LEVEL_MESSAGE)
		falcon_log_level |= G_LOG_LEVEL_MESSAGE;
	if (log_level >= G_LOG_LEVEL_INFO)
		falcon_log_level |= G_LOG_LEVEL_INFO;
	if (log_level >= G_LOG_LEVEL_DEBUG)
		falcon_log_level |= G_LOG_LEVEL_DEBUG;
}

gint falcon_object_compare(gconstpointer a, gconstpointer b)
{
	return g_strcmp0(falcon_object_get_name((const falcon_object_t *)a),
	                 (const gchar *)b);
}

void falcon_error_report(GError *error)
{
	if (error) {
		switch (error->code) {
		case FALCON_ERROR_ERROR:
			g_error("%s: %s", g_quark_to_string(error->domain),
			        error->message);
			break;
		case FALCON_ERROR_CRITICAL:
			g_critical("%s: %s", g_quark_to_string(error->domain),
			           error->message);
			break;
		case FALCON_ERROR_WARNING:
			g_warning("%s: %s", g_quark_to_string(error->domain),
			          error->message);
			break;
		default:
			g_message("%s: %s", g_quark_to_string(error->domain),
			          error->message);
		}
	}
}
