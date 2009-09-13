#include <glib.h>

#include "falcon.h"
#include "handler.h"
#include "object.h"
#include "events.h"
#include "common.h"

int main(int argc ATTRIBUTE_UNUSED, char **argv) {
	g_thread_init(NULL);
	falcon_init();
	falcon_set_log_level(G_LOG_LEVEL_DEBUG);

	falcon_add(argv[1], FALSE);
	falcon_shutdown();

	return 0;
}
