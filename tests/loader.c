#include <stdio.h>
#include <sys/stat.h>
#include <glib.h>

#include "cache.h"

static void print_cache(falcon_object_t *object,
                        gpointer data ATTRIBUTE_UNUSED) {
	printf("%s: %s (%ld bytes, %ld secs, %s)\n",
	       falcon_object_get_name(object),
	       S_ISDIR(object->mode) ? "dir" : "file",
	       object->size,
	       object->time,
	       object->watch ? "yes" : "no");
}

int main(int argc ATTRIBUTE_UNUSED, char **argv) {
	g_thread_init(NULL);

	falcon_cache_t *cache = falcon_cache_new();

	if (!falcon_cache_load(cache, argv[1]))
		printf("failed\n");

	printf("length: %d\n", g_queue_get_length(cache->objects));

	g_queue_foreach(cache->objects, (GFunc)print_cache, NULL);

	return 0;
}
