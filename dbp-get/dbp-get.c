#include <stdio.h>
#include <string.h>
#include "dbp-get.h"

#include <dbpmgr/package_list.h>

void usage() {
	return;
}


int main(int argc, char **argv) {
	struct DBPPackageList *list;
	int i;
	
	if (argc < 2) {
		return usage(), 1;
	}

	if (!strcmp(argv[1], "update")) {

		list = dbp_pkglist_new("armhf");
		for (i = 0; i < list->source_ids; i++) {
			fprintf(stdout, "--> %s\n", list->source_id[i].url);
			dbp_pkglist_cache_update_one(list, i);
		}
	}

	return 0;
}
