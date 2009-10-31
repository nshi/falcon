#include <glib.h>
#include <signal.h>

#include "include/falcon.h"

GMainLoop *ml = NULL;

gboolean test_handler(falcon_object_t *object, falcon_event_code_t event,
                      gpointer userdata __attribute__((unused))) {
	g_message("custom handler: %s->%s",
	          falcon_object_get_name(object), falcon_event_to_string(event));
	return TRUE;
}

void ex(int sig __attribute__((unused)))
{
	g_main_loop_quit(ml);
}

int main(int argc __attribute__((unused)), char **argv)
{
	g_thread_init(NULL);
	falcon_init("cache.dat");

	falcon_set_log_level(G_LOG_LEVEL_MESSAGE);
	falcon_handler_register(EVENT_ALL, test_handler, NULL);
	falcon_filter_register(TRUE, "skip.*", NULL);
	falcon_filter_register(FALSE, "/skip.*", NULL);

	falcon_add(argv[1], TRUE);

	signal(SIGINT, ex);
	ml = g_main_loop_new (NULL, FALSE);

	g_main_loop_run(ml);
	g_main_loop_unref(ml);

	falcon_shutdown("cache.dat", TRUE);

	return 0;
}
