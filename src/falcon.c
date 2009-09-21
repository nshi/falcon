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

#include "falcon.h"
#include "handler.h"
#include "watcher.h"

typedef struct {
	GMutex *lock;
	GQueue pending_objects;
	GQueue failed_objects;
	falcon_cache_t *cache;
	GThreadPool *walkers;
	GCond *running_cond;
	guint running;				/* Currently scheduled tasks */
} falcon_context_t;

static falcon_context_t context;

static void falcon_context_init(void) {
	context.lock = g_mutex_new();
	g_queue_init(&context.pending_objects);
	g_queue_init(&context.failed_objects);
	context.cache = falcon_cache_new();
	context.walkers = g_thread_pool_new(falcon_walker_run,
	                                    context.cache,
	                                    MAX_WALKERS,
	                                    TRUE,
	                                    NULL);
	context.running_cond = g_cond_new();
	context.running = 0;
}

static void falcon_context_free(gboolean wait) {
	falcon_object_t *object = NULL;

	while ((object = g_queue_pop_head(&context.pending_objects))) {
		falcon_object_free(object);
	}
	while ((object = g_queue_pop_head(&context.failed_objects))) {
		falcon_object_free(object);
	}
	g_thread_pool_free(context.walkers, !wait, wait);
	g_cond_free(context.running_cond);
	g_mutex_free(context.lock);
	falcon_cache_free(context.cache);
}

/* The caller must lock the context. */
static void falcon_push(GQueue *queue, falcon_object_t *object) {
	GList *l = NULL;

	g_return_if_fail(queue);

	l = g_queue_find_custom(queue, object->name, falcon_object_compare);
	if (!l)
		g_queue_push_tail(queue, object);
}

/* The caller must lock the context. */
static void falcon_dispatch(gboolean force) {
	GQueue *objects = NULL;
	guint length = g_queue_get_length(&context.pending_objects);

	if (length == 0)
		return;

	g_debug(_("Dispatching conditions: force (%s), length (%d), running (%d)."),
	        force ? "true" : "false",
	        length,
	        context.running);
	if (force
	    || length == OBJECTS_PER_THREAD
	    || context.running == 0) {
		objects = g_queue_copy(&context.pending_objects);
		g_queue_clear(&context.pending_objects);
		g_debug(_("Dispatching %d objects to a walker."),
		        g_queue_get_length(objects));
	} else {
		g_debug(_("%d objects pending."), length);
	}

	if (objects) {
		g_thread_pool_push(context.walkers, objects, NULL);
		context.running++;
	}
}

static gchar *falcon_normalize_path(const gchar *name) {
	gchar *path = NULL;

	g_return_val_if_fail(name, NULL);

	if (g_str_has_suffix(name, G_DIR_SEPARATOR_S))
		path = g_strndup(name, strlen(name) - 1);
	else
		path = g_strdup(name);

	return path;
}

static void falcon_start_one(gpointer data, gpointer userdata ATTRIBUTE_UNUSED) {
	const falcon_object_t *object = (const falcon_object_t *)data;
	falcon_object_t *tmp = falcon_object_copy(object);
	falcon_task_add(tmp);
}

static void falcon_start_all(void) {
	/* Wait until trie is implemented. */
	falcon_cache_foreach(context.cache, falcon_start_one, NULL);
}

void falcon_init(const gchar *name) {
	g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, falcon_log_handler, NULL);

	falcon_context_init();
	falcon_handler_registry_init();
	falcon_watcher_init(context.cache);

	falcon_cache_load(context.cache, name);
	falcon_start_all();
}

void falcon_shutdown(const gchar *name, gboolean wait) {
	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return;
	}

	if (wait) {
		g_mutex_lock(context.lock);
		while (context.running != 0
		       || g_queue_get_length(&context.pending_objects) > 0) {
			if (g_queue_get_length(&context.pending_objects) > 0)
				falcon_dispatch(TRUE);
			g_cond_wait(context.running_cond, context.lock);
		}
		g_mutex_unlock(context.lock);
	}

	falcon_cache_save(context.cache, name);

	falcon_watcher_shutdown();
	falcon_handler_registry_shutdown();
	/* Maybe we have to save the cache before freeing. */
	falcon_context_free(wait);
}

void falcon_add(const gchar *name, gboolean watch) {
	falcon_object_t *object = NULL;
	gchar *path = NULL;

	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return;
	}

	if (!name) {
		g_warning(_("Failed to add object, name not provided."));
		return;
	}

	path = falcon_normalize_path(name);

	g_debug(_("Adding \"%s\" by name."), path);

	g_mutex_lock(context.lock);
	object = falcon_cache_get(context.cache, path);
	g_mutex_unlock(context.lock);
	if (!object) {
		object = falcon_object_new(path);
		falcon_object_set_watch(object, watch);
		falcon_task_add(object);
	}

	g_free(path);
}

gboolean falcon_set_watch(const gchar *name, gboolean watch) {
	falcon_object_t *object = NULL;
	gchar *path = NULL;
	gboolean ret = FALSE;

	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return FALSE;
	}

	if (!name) {
		g_warning(_("Failed to set watchability flag,"
		            " object name not provided."));
		return FALSE;
	}

	path = falcon_normalize_path(name);

	g_debug(_("Setting \"%s\" watchability to %s."), name,
	        watch ? _("true") : _("false"));

	g_mutex_lock(context.lock);
	object = falcon_cache_get(context.cache, path);
	g_mutex_unlock(context.lock);
	g_free(path);
	if (!object) {
		g_warning(_("Failed to set watchability flag,"
		            " object %s does not exist."), name);
		return FALSE;
	}

	falcon_object_set_watch(object, watch);
	if (watch)
		ret = falcon_watcher_add(object);
	else
		ret = falcon_watcher_delete(object);

	if (!ret)
		g_warning(_("Failed to set watchability flag for \"%s\"."), name);

	return ret;
}

void falcon_task_add(falcon_object_t *object) {
	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return;
	}

	g_return_if_fail(object);
	g_debug(_("Adding task \"%s\"."), object->name);

	g_mutex_lock(context.lock);
	falcon_push(&context.pending_objects, object);
	falcon_dispatch(FALSE);
	g_mutex_unlock(context.lock);
}

void falcon_failed_add(falcon_object_t *object) {
	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return;
	}

	g_return_if_fail(object);
	g_mutex_lock(context.lock);
	falcon_push(&context.failed_objects, object);
	g_mutex_unlock(context.lock);
}

void falcon_walker_return(GError *error) {
	if (!context.lock || !context.cache || !context.walkers
	    || !context.running_cond) {
		g_critical(_("Please initialize the system first."));
		return;
	}

	falcon_error_report(error);

	g_mutex_lock(context.lock);
	context.running--;
	g_cond_signal(context.running_cond);
	falcon_dispatch(FALSE);
	g_mutex_unlock(context.lock);
}
