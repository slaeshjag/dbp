#include <stdio.h>
#include <string.h>
#include "dbp-get.h"

#include <dbpmgr/dbpmgr.h>
#include <dbpmgr/package_list.h>

void usage() {
	return;
}


int main(int argc, char **argv) {
	struct DBPPackageList **list;
	int lists;
	int i, j;
	
	if (argc < 2) {
		return usage(), 1;
	}

	dbp_mgr_init();

	if (!strcmp(argv[1], "update")) {
		dbp_pkglist_arch_supported_load(&list, &lists);
		for (j = 0; j < lists; j++)
			for (i = 0; i < list[j]->source_ids; i++) {
				fprintf(stdout, "--> %s\n", list[j]->source_id[i].url);
				dbp_pkglist_cache_update_one(list[j], i);
			}
	} else if (!strcmp(argv[1], "list")) {
		int k;
		struct DBPPackageList **rec;
		/* TODO: Make sure we print out the latest version, and no duplicates due to various branches */
		dbp_pkglist_arch_supported_load(&list, &lists);
		dbp_pkglist_cache_read_all(list, lists);
		dbp_pkglist_recommended_select(&rec, list, lists);

		for (j = 0; j < lists; j++)
			for (i = 0; i < rec[j]->branches; i++)
				for (k = 0; k < rec[j]->branch[i].ids; k++)
					fprintf(stdout, "%s:%s - %s\n", rec[j]->branch[i].id[k].pkg_id, rec[j]->arch, rec[j]->branch[i].id[k].version->description.shortdesc); 
	}

	return 0;
}
