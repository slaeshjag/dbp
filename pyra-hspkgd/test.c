#include <stdio.h>
#include <stdlib.h>
#include "mountwatch.h"

int main(int argc, char **argv) {
	struct mountwatch_change_s change;
	int i;

	if (!mountwatch_init())
		exit(-1);
	
	for (;;) {
		change = mountwatch_diff();
		for (i = 0; i < change.entries; i++) {
			switch (change.entry[i].tag) {
				case MOUNTWATCH_TAG_REMOVED:
					fprintf(stderr, "%s umounted from %s\n", change.entry[i].device, change.entry[i].mount);
					break;
				case MOUNTWATCH_TAG_ADDED:
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
