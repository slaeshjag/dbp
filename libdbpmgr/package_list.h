/*
Copyright (c) 2016 Steven Arnow <s@rdw.se>
'package_list.h' - This file is part of libdbpmgr

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/


#ifndef __DBPMGR_PACKAGE_LIST_H__
#define	__DBPMGR_PACKAGE_LIST_H__

#include <time.h>
#include <dbpbase/desktop.h>


struct DBPPackageLocale {
	char				*name;
	char				*shortdesc;
	char				*locale;
	void				*additional_data; /* Reserved for future use */
};


struct DBPPackageCategory {
	char				*main;
	char				*sub;
	char				*subsub;
};


struct DBPPackageVersion {
	char				*version;
	int				feed_id;
	struct DBPPackageLocale		description;
	struct DBPPackageLocale		*locale;
	int				locales;

	struct DBPPackageCategory	*category;
	int				categories;

	struct {
		char			*any;
		char			*deb;
		char			*pref_deb;
		char			*dbp;
		char			*pref_dbp;
	} dep;

	char				*icon_url;
	void				*additional_data; /* Reseved for future use */
};


struct DBPPackageListID {
	char				*pkg_id;
	struct DBPPackageVersion	*version;
	int				versions;
};


struct DBPPackageListBranch {
	char				*name;
	struct DBPPackageListID		*id;
	int				ids;
};


struct DBPPackageSourceID {
	char				*name;
	char				*url;
	time_t				last_update;
};


/* One package list per arch, please */
struct DBPPackageList {
	struct DBPPackageListBranch	*branch;
	int				branches;
	struct DBPPackageSourceID	*source_id;
	int				source_ids;
	char				*arch;
};


int dbp_pkglist_source_add(struct DBPPackageList *list, const char *name, const char *url);
struct DBPPackageList *dbp_pkglist_new(const char *arch);
struct DBPPackageList *dbp_pkglist_free(struct DBPPackageList *list);
void dbp_pkglist_parse(struct DBPPackageList *list, const char *branch, int source_id, struct DBPDesktopFile *pkglist);
int dbp_pkglist_branch_add(struct DBPPackageList *list, const char *branch);

char *dbp_pkglist_source_cache_path(struct DBPPackageList *list, int source_id);
void dbp_pkglist_cache_read(struct DBPPackageList *list);
void dbp_pkglist_cache_read_all(struct DBPPackageList **list, int lists);

void dbp_pkglist_cache_update_one(struct DBPPackageList *list, int source_id);
void dbp_pkglist_cache_update(struct DBPPackageList *list);

void dbp_pkglist_arch_supported_load(struct DBPPackageList ***list, int *lists);

#endif
