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

static int daemon_nuke() {
	if (!daemon_nuke_dir(config_struct.desktop_directory));
	else if (!daemon_nuke_dir(config_struct.icon_directory));
	else
		return 1;
	return 0;
}


static int daemon_init() {
	if (!loop_directory_setup(config_struct.img_mount, 0755));
	else if (!loop_directory_setup(config_struct.union_mount, 0777));
	else if (!loop_directory_setup(config_struct.icon_directory, 0755));
	else if (!loop_directory_setup(config_struct.exec_directory, 0755));
	else if (!loop_directory_setup(config_struct.desktop_directory, 0755));
	else if (!daemon_nuke());
	else
		return 1;
	return 0;
}


int main(int argc, char **argv) {
	struct mountwatch_change_s change;
	struct package_s p;
	int i;

	p = package_init();
	comm_dbus_register(&p);

	if (!mountwatch_init())
		exit(-1);
	config_init();
	if (!daemon_init())
		exit(-1);
	
	for (;;) {
		change = mountwatch_diff();
		for (i = 0; i < change.entries; i++) {
			switch (change.entry[i].tag) {
				case MOUNTWATCH_TAG_REMOVED:
					package_release_mount(&p, change.entry[i].device);
					fprintf(stderr, "%s umounted from %s\n", change.entry[i].device, change.entry[i].mount);
					break;
				case MOUNTWATCH_TAG_ADDED:
					package_crawl_mount(&p, change.entry[i].device, change.entry[i].mount);
					fprintf(stderr, "%s mounted at %s\n", change.entry[i].device, change.entry[i].mount);
					break;
				default:
					break;
			}
		}

		mountwatch_change_free(change);
	}

	return 0;
}
