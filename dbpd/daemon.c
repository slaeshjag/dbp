#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include "mountwatch.h"
#include "config.h"
#include "package.h"
#include "dbp.h"
#include "comm.h"
#include "loop.h"

FILE *dbp_error_log;

static int daemon_nuke_dir(char *dir) {
	DIR *d;
	struct dirent de, *result;
	char path[PATH_MAX];
	
	if (!(d = opendir(dir)))
		return 0;
	for (readdir_r(d, &de, &result); result; readdir_r(d, &de, &result))
		if (strstr(de.d_name, DBP_META_PREFIX) == de.d_name)
			sprintf(path, "%s/%s", dir, de.d_name), unlink(path);
	closedir(d);
	return 1;
}


static int daemon_nuke_execs() {
	DIR *d;
	struct dirent de, *result;
	FILE *fp;
	char buff[512], path[PATH_MAX];

	if (!(d = opendir(config_struct.exec_directory)))
		return 0;
	for (readdir_r(d, &de, &result); result; readdir_r(d, &de, &result)) {
		sprintf(path, "%s/%s", config_struct.exec_directory, de.d_name);
		if (!(fp = fopen(path, "r")))
			continue;
		*buff = 0;
		fgets(buff, 512, fp);
		fgets(buff, 512, fp);
		fclose(fp);
		buff[511] = 0;
		if (strcmp(buff, "#dbp-template\n"))
			continue;
		unlink(path);
	}

	closedir(d);

	return 1;
}


static int daemon_nuke() {
	if (!daemon_nuke_dir(config_struct.desktop_directory));
	else if (!daemon_nuke_dir(config_struct.icon_directory));
	else if (!daemon_nuke_execs());
	else
		return 1;
	return 0;
}


static int daemon_init() {
	if (!loop_directory_setup(config_struct.img_mount, 0755));
	else if (!loop_directory_setup(config_struct.union_mount, 0755));
	else if (!loop_directory_setup(config_struct.icon_directory, 0755));
	else if (!loop_directory_setup(config_struct.exec_directory, 0755));
	else if (!loop_directory_setup(config_struct.desktop_directory, 0755));
	else if (!loop_directory_setup(config_struct.dbpout_directory, 0777));
	else if (!daemon_nuke());
	else
		return 1;
	return 0;
}


int main(int argc, char **argv) {
	struct mountwatch_change_s change;
	struct package_s p;
	int i;
	char *n;

	dbp_error_log = stderr;
	if (!config_init())
		return -1;
	if (!(dbp_error_log = fopen(config_struct.daemon_log, "w"))) {
		dbp_error_log = stderr;
		fprintf(stderr, "Unable to open %s\n", config_struct.daemon_log);
	} else
		setbuf(dbp_error_log, NULL);
	p = package_init();
	comm_dbus_register(&p);

	if (!mountwatch_init())
		exit(-1);
	if (!daemon_init())
		exit(-1);
	
	for (;;) {
		change = mountwatch_diff();
		for (i = 0; i < change.entries; i++) {
			switch (change.entry[i].tag) {
				case MOUNTWATCH_TAG_REMOVED:
					package_release_mount(&p, change.entry[i].device);
					break;
				case MOUNTWATCH_TAG_ADDED:
					package_crawl_mount(&p, change.entry[i].device, change.entry[i].mount);
					break;
				case MOUNTWATCH_TAG_PKG_ADDED:
					package_register_path(&p, change.entry[i].device, change.entry[i].path, change.entry[i].mount, &n);
					free(n);
					break;
				case MOUNTWATCH_TAG_PKG_REMOVED:
					package_release_path(&p, change.entry[i].path);
					break;
				default:
					break;
			}
		}

		mountwatch_change_free(change);
	}

	(void) argc, (void) argv;

	return 0;
}
