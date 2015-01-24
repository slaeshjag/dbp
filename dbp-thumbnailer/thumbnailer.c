#include <stdio.h>
#include "thumb_queue.h"

FILE *dbp_error_log;

int main(int argc, char **argv) {
	dbp_error_log = stderr;
	thumb_queue_init();
	comm_dbus_loop();
}
