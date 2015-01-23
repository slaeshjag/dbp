#include <stdio.h>

FILE *dbp_error_log;

int main(int argc, char **argv) {
	dbp_error_log = stderr;
	comm_dbus_loop();
}
