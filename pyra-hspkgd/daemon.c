#include <stdio.h>
#include <stdlib.h>
#include "mountwatch.h"
#include "config.h"
#include "package.h"
#include "dbp.h"
#include "comm.h"
#include "loop.h"


static int daemon_init() {
	if (!loop_directory_setup(config_struct.img_mount, 0755));
	else if (!loop_directory_setup(config_struct.union_mount, 0777));
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
