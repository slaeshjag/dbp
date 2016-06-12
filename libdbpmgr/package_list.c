/*
Copyright (c) 2016 Steven Arnow <s@rdw.se>
'package_list.c' - This file is part of libdbpmgr

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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dbpbase/desktop.h>
#include <dbpbase/config.h>

#include "categories.h"
#include "dependencies.h"
#include "package_list.h"


static int _locate_pkgid(struct DBPPackageList *list, int branch_id, const char *pkg_id) {
	int i;

	for (i = 0; i < list->branch[branch_id].ids; i++)
		if (!strcmp(list->branch[branch_id].id[i].pkg_id, pkg_id))
			return i;
	return -1;
}


static int _locate_pkg_version(struct DBPPackageList *list, int branch_id, int pkgid, const char *version) {
	int i;

	for (i = 0; i < list->branch[branch_id].id[pkgid].versions; i++)
		if (!strcmp(list->branch[branch_id].id[pkgid].version[i].version, version))
			return i;
	return -1;
}


static void _add_to_locale(struct DBPPackageVersion *pid, const char *locale, const char *name, const char *desc) {
	int i;

	for (i = 0; i < pid->locales; i++)
		if (!strcmp(pid->locale[i].locale, locale))
			break;
	if (i == pid->locales) {
		pid->locales++;
		pid->locale = realloc(pid->locale, sizeof(pid->locale) * pid->locales);
		pid->locale[i].locale = strdup(locale);
		pid->locale[i].name = NULL, pid->locale[i].shortdesc = NULL, pid->locale[i].additional_data = NULL;
	}

	if (name) {
		free(pid->locale[i].name), pid->locale[i].name = strdup(name);
		free(pid->locale[i].shortdesc), pid->locale[i].shortdesc = strdup(desc);
	}

	return;
}


static bool _valid_category(const char *prev, const char *cat) {
	int i, j;

	for (i = 0; freedesktop_cat[i].name; i++) {
		if (strcmp(freedesktop_cat[i].name, cat))
			continue;
		if (!freedesktop_cat[i].requires_parent && prev)
			return false;
		if (!freedesktop_cat[i].requires_parent)
			return true;
		if (!prev)
			return false;
		for (j = 0; freedesktop_cat[i].parents[j]; j++)
			if (!strcmp(freedesktop_cat[i].parents[j], prev))
				return true;
		return false;
	}

	return false;
}


static void _add_category(struct DBPPackageVersion *pkg, char *main, char *sub, char *subsub) {
	int id;

	id = pkg->categories++;
	pkg->category = realloc(pkg->category, sizeof(*pkg->category) * pkg->categories);
	pkg->category[id].main = main;
	pkg->category[id].sub = sub;
	pkg->category[id].subsub = subsub;

	return;
}


static void _process_categories(struct DBPPackageVersion *pkg, const char *categories) {
	char **cat_arr;
	int cats, i;
	char *new_cat = strdup(categories);;
	if (pkg->category)
		return;	// We have already added categories
	dbp_config_expand_token(&cat_arr, &cats, new_cat);
	free(new_cat);

	for (i = 0; i < cats; i++) {
		if (_valid_category(NULL, cat_arr[i])) {
			if (i + 1 < cats && _valid_category(cat_arr[i], cat_arr[i+1])) {
				if (i + 2 < cats && _valid_category(cat_arr[i+1], cat_arr[i+2])) {	/* I don't *think* there's ever more than 3 levels of categories... */
					_add_category(pkg, cat_arr[i], cat_arr[i+1], cat_arr[i+2]), i += 2;
				} else
					_add_category(pkg, cat_arr[i], cat_arr[i+1], NULL), i += 1;
			} else
				_add_category(pkg, cat_arr[i], NULL, NULL);
		}
	}

	free(cat_arr);
	return;
}


int dbp_pkglist_branch_add(struct DBPPackageList *list, const char *branch) {
	int i;

	for (i = 0; i < list->branches; i++)
		if (!strcmp(branch, list->branch[i].name))
			return i;
	list->branches++;
	list->branch = realloc(list->branch, sizeof(*list->branch) * list->branches);
	list->branch[i].name = strdup(branch);
	list->branch[i].id = NULL, list->branch[i].ids = 0;
	return i;
}


void dbp_pkglist_parse(struct DBPPackageList *list, const char *branch, int source_id, struct DBPDesktopFile *pkglist) {
	int i, branch_id, j;

	branch_id = dbp_pkglist_branch_add(list, branch);
	
	for (i = 0; i < pkglist->sections; i++) {
		int pkgid, pkgver_index, pkgid_index, verid;

		if ((pkgid_index = dbp_desktop_lookup_entry(pkglist, "ID", "", i)) < 0)
			continue; // No ID, no add
		if ((pkgver_index = dbp_desktop_lookup_entry(pkglist, "Version", "", i)) < 0)
			continue; // No version, no add

		if ((pkgid = _locate_pkgid(list, branch_id, pkglist->section[i].entry[pkgid_index].value)) < 0) {
			pkgid = list->branch[branch_id].ids++;
			list->branch[branch_id].id = realloc(list->branch[branch_id].id, list->branch[branch_id].ids * sizeof(*list->branch[branch_id].id));
			list->branch[branch_id].id[pkgid].pkg_id = strdup(pkglist->section[i].entry[pkgid_index].value);
			list->branch[branch_id].id[pkgid].version = NULL, list->branch[branch_id].id[pkgid].versions = 0;
		} else if (_locate_pkg_version(list, branch_id, pkgid, pkglist->section[i].entry[pkgver_index].value) >= 0)
			continue; // We already have this version, move along... */

		verid = list->branch[branch_id].id[pkgid].versions++;
		list->branch[branch_id].id[pkgid].version = realloc(list->branch[branch_id].id[pkgid].version, sizeof(*list->branch[branch_id].id[pkgid].version) * list->branch[branch_id].id[pkgid].versions);
		memset(&list->branch[branch_id].id[pkgid].version[verid], 0, sizeof(list->branch[branch_id].id[pkgid].version[verid]));
	
		/* TODO: Snygga upp det här spaghettimonstret... */
		for (j = 0; j < pkglist->section[i].entries; j++) {
			if (strcmp(pkglist->section[i].entry[j].locale, "")) {
				if (!strcmp(pkglist->section[i].entry[j].key, "Name"))
					_add_to_locale(&list->branch[branch_id].id[pkgid].version[verid], pkglist->section[i].entry[j].locale, pkglist->section[i].entry[j].value, NULL);
				else if (!strcmp(pkglist->section[i].entry[j].key, "Description"))
					_add_to_locale(&list->branch[branch_id].id[pkgid].version[verid], pkglist->section[i].entry[j].locale, NULL, pkglist->section[i].entry[j].value);
				else if (!strcmp(pkglist->section[i].entry[j].key, "Dependency")) {
					if (!strcmp(pkglist->section[i].entry[j].locale, "deb"))
						free(list->branch[branch_id].id[pkgid].version[verid].dep.deb), list->branch[branch_id].id[pkgid].version[verid].dep.deb = strdup(pkglist->section[i].entry[j].value);
					if (!strcmp(pkglist->section[i].entry[j].locale, "pref_deb"))
						free(list->branch[branch_id].id[pkgid].version[verid].dep.pref_deb), list->branch[branch_id].id[pkgid].version[verid].dep.pref_deb = strdup(pkglist->section[i].entry[j].value);
					if (!strcmp(pkglist->section[i].entry[j].locale, "dbp"))
						free(list->branch[branch_id].id[pkgid].version[verid].dep.dbp), list->branch[branch_id].id[pkgid].version[verid].dep.dbp = strdup(pkglist->section[i].entry[j].value);
					if (!strcmp(pkglist->section[i].entry[j].locale, "pref_dbp"))
						free(list->branch[branch_id].id[pkgid].version[verid].dep.pref_dbp), list->branch[branch_id].id[pkgid].version[verid].dep.pref_dbp = strdup(pkglist->section[i].entry[j].value);
				}
			} else if (!strcmp(pkglist->section[i].entry[j].key, "Name"))
				free(list->branch[branch_id].id[pkgid].version[verid].description.name), list->branch[branch_id].id[pkgid].version[verid].description.name = strdup(pkglist->section[i].entry[j].value);
			else if (!strcmp(pkglist->section[i].entry[j].key, "Description"))
				free(list->branch[branch_id].id[pkgid].version[verid].description.shortdesc), list->branch[branch_id].id[pkgid].version[verid].description.shortdesc = strdup(pkglist->section[i].entry[j].value);
			else if (!strcmp(pkglist->section[i].entry[j].key, "Version"))
				free(list->branch[branch_id].id[pkgid].version[verid].version), list->branch[branch_id].id[pkgid].version[verid].version = strdup(pkglist->section[i].entry[j].value);
			else if (!strcmp(pkglist->section[i].entry[j].key, "IconURL"))
				free(list->branch[branch_id].id[pkgid].version[verid].icon_url), list->branch[branch_id].id[pkgid].version[verid].icon_url = strdup(pkglist->section[i].entry[j].value);
			else if (!strcmp(pkglist->section[i].entry[j].key, "Categories"))
				_process_categories(&list->branch[branch_id].id[pkgid].version[verid], pkglist->section[i].entry[j].value);
			else if (!strcmp(pkglist->section[i].entry[j].key, "Dependency"))
				free(list->branch[branch_id].id[pkgid].version[verid].dep.any), list->branch[branch_id].id[pkgid].version[verid].dep.any = strdup(pkglist->section[i].entry[j].value);
		}

		list->branch[branch_id].id[pkgid].version[verid].feed_id = source_id;
	}

	return;
}
