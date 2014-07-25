#include <stdio.h>
#include <stdlib.h>
#include "mountwatch.h"
#include "config.h"
#include "package.h"

int main(int argc, char **argv) {
	struct mountwatch_change_s change;
	struct package_s p;
	int i;

	p = package_init();

	if (!mountwatch_init())
		exit(-1);
	config_init();
	
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
