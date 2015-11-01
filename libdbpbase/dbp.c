#define _LIBINTERNAL
#include "dbp.h"
#include "config.h"
#include <stdio.h>

int dbp_config_init();

FILE *dbp_error_log;

int dbp_init(FILE *error_log) {
	if (!error_log)
		dbp_error_log = stderr;
	else
		dbp_error_log = error_log;
	if (!dbp_config_init())
		return 0;
	return 1;
}
