#define	_GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbpmgr.h"


char *dbp_mgr_cache_directory() {
	const char *base;
	char *cachedir;

	if (!(base = getenv("XDG_CACHE_HOME"))) {
		if (!(base = getenv("HOME")))
			cachedir = strdup("/tmp/dbpmgr/cache");
		else
			asprintf(&cachedir, "%s/.dbpmgr/cache", base);
	} else
		asprintf(&cachedir, "%s/dbpmgr", base);
	return cachedir;
}


char *dbp_mgr_config_directory() {
	const char *base;
	char *confdir;

	if (!(base = getenv("XDG_CONFIG_HOME"))) {
		if (!(base = getenv("HOME")))
			confdir = strdup("/tmp/dbpmgr/config");
		else
			asprintf(&confdir, "%s/.dbpmgr/config", base);
	} else
		asprintf(&confdir, "%s/dbpmgr", base);
	return confdir;
}
