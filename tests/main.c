#include <glib.h>

#include "falcon.h"
#include "handler.h"
#include "object.h"
#include "events.h"
#include "common.h"

gboolean test_handler(falcon_object_t *object, falcon_event_code_t event,
                      gpointer userdata ATTRIBUTE_UNUSED) {
	g_message("custom handler: %s->%s",
	          object->name, falcon_event_to_string(event));
	return TRUE;
}

int main(int argc ATTRIBUTE_UNUSED, char **argv) {
	GMainLoop *ml = NULL;

	g_thread_init(NULL);
	falcon_init();

	falcon_set_log_level(G_LOG_LEVEL_MESSAGE);
	falcon_handler_register(EVENT_ALL, test_handler, NULL);

	falcon_add(argv[1], TRUE);

	ml = g_main_loop_new (NULL, FALSE);

	g_main_loop_run(ml);
	g_main_loop_unref(ml);
	falcon_shutdown(TRUE);

	return 0;
}
