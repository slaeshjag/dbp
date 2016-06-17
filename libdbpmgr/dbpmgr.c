#define	_GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../libdbpbase/dbpbase.h"
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


int dbp_mgr_file_lock(const char *lockfile) {
	int fd;

	fd = open(lockfile, O_CREAT | O_RDWR, 0600);
	flock(fd, LOCK_EX);
	return fd;
}


int dbp_mgr_file_unlock(int fd) {
	flock(fd, LOCK_UN);
	return -1;
}


int dbp_mgr_mkdir_recursive(const char *path, int mode) {
	char *path_tmp, *saveptr, *next;
	char *build_old, *build_new;
	struct stat st;

	build_old = strdup("");
	path_tmp = strdup(path);
	for (next = strtok_r(path_tmp, "/", &saveptr); next; next = strtok_r(NULL, "/", &saveptr)) {
		asprintf(&build_new, "%s/%s", build_old, next);
		free(build_old);
		build_old = build_new;
		if (stat(build_old, &st) < 0) {
			if (mkdir(build_old, mode) < 0)
				return -1;
		} else {
			if (!(st.st_mode & S_IFDIR))
				return -1;
		}
	}

	free(path_tmp);
	free(build_old);
	return 0;
}


int dbp_mgr_init() {
	return dbp_config_init();
}
