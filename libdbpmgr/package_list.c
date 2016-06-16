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
#define	_GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <sys/stat.h>
#include <glib.h>
#include <dbpbase/desktop.h>
#include <dbpbase/config.h>
#include <curl/curl.h>
#include <zlib.h>

#include "dbpmgr.h"
#include "categories.h"
#include "dependencies.h"
#include "package_list.h"

static void _get_repo_line_info(const char *repoline, char **url, char **arch, char **branch, char **secret);


static int _obtain_pkg_cache_lock() {
	char *cache_path, *lockpath;
	int lock;

	cache_path = dbp_mgr_cache_directory();
	asprintf(&lockpath, "%s/pkglist.lock", cache_path);
	free(cache_path);
	if ((lock = dbp_mgr_file_lock(lockpath)) < 0)
		fprintf(stderr, "Unable to obtain package list cache lock %s\n", lockpath);

	free(lockpath);
	return lock;
}

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
			if (!strcmp(pkglist->section[i].name, "")) {
				if (!strcmp(pkglist->section[i].entry[j].key, "FeedName"))
					free(list->source_id[source_id].name), list->source_id[source_id].name = strdup(pkglist->section[i].entry[j].value);
			}
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

struct DBPPackageList *dbp_pkglist_free(struct DBPPackageList *list) {
	int i, j, k, l;

	free(list->arch);

	for (i = 0; i < list->source_ids; i++) {
		free(list->source_id[i].url);
		free(list->source_id[i].name);
	}
	free(list->source_id);

	for (i = 0; i < list->branches; i++) {
		free(list->branch[i].name);
		for (j = 0; j < list->branch[i].ids; i++) {
			free(list->branch[i].id[j].pkg_id);
			for (k = 0; k < list->branch[i].id[j].versions; k++) {
				free(list->branch[i].id[j].version[k].version);
				free(list->branch[i].id[j].version[k].dep.any);
				free(list->branch[i].id[j].version[k].dep.deb);
				free(list->branch[i].id[j].version[k].dep.pref_deb);
				free(list->branch[i].id[j].version[k].dep.dbp);
				free(list->branch[i].id[j].version[k].dep.pref_dbp);
				free(list->branch[i].id[j].version[k].icon_url);
				
				free(list->branch[i].id[j].version[k].description.name);
				free(list->branch[i].id[j].version[k].description.shortdesc);
				free(list->branch[i].id[j].version[k].description.locale);

				for (l = 0; l < list->branch[i].id[j].version[k].locales; l++) {
					free(list->branch[i].id[j].version[k].locale[l].name);
					free(list->branch[i].id[j].version[k].locale[l].shortdesc);
					free(list->branch[i].id[j].version[k].locale[l].locale);
				}
				free(list->branch[i].id[j].version[k].locale);

				for (l = 0; l < list->branch[i].id[j].version[k].categories; l++) {
					free(list->branch[i].id[j].version[k].category[l].main);
					free(list->branch[i].id[j].version[k].category[l].sub);
					free(list->branch[i].id[j].version[k].category[l].subsub);
				}
				free(list->branch[i].id[j].version[k].category);
			}
			free(list->branch[i].id[j].version);
		}
		free(list->branch[i].id);
	}
	free(list->branch);
	return NULL;
}


char *dbp_pkglist_sourcelist_path() {
	char *config_dir, *path;

	config_dir = dbp_mgr_config_directory();
	asprintf(&path, "%s/sources.list", config_dir);
	free(config_dir);
	return path;
}


struct DBPPackageList *dbp_pkglist_new(const char *arch) {
	struct DBPPackageList *list;
	char *sourcelist;

	list = malloc(sizeof(*list));
	list->branch = NULL, list->branches = 0;
	list->source_id = NULL, list->source_ids = 0;
	list->arch = strdup(arch);

	sourcelist = dbp_pkglist_sourcelist_path();
	/* Read source list */ {
		FILE *fp;
		char buff[4096];
		char *arch;
		if (!(fp = fopen(sourcelist, "r"))) {
			fprintf(stderr, "Could not open the source list '%s'\n", sourcelist);
			goto end;
		}

		while (!feof(fp)) {
			*buff = 0;
			fgets(buff, 4096, fp);
			if (!*buff)
				continue;
			while (strchr(buff, '\n'))
				*strchr(buff, '\n') = 0;
			
			_get_repo_line_info(buff, NULL, &arch, NULL, NULL);
			if (strcmp(arch, list->arch)) {
				free(arch);
				continue;
			}

			dbp_pkglist_source_add(list, buff, buff); // We don't have the feed name yet
		}
	}
end:
	return list;
}


int dbp_pkglist_source_add(struct DBPPackageList *list, const char *name, const char *url) {
	int i;

	i = list->source_ids++;
	list->source_id = realloc(list->source_id, sizeof(*list->source_id) * list->source_ids);
	list->source_id[i].url = strdup(url);
	list->source_id[i].name = strdup(name);

	return i;
}


static void _get_repo_line_info(const char *repoline, char **url, char **arch, char **branch, char **secret) {
	char *_url, *_arch, *_branch, *_secret;
	_secret = NULL;
	sscanf(repoline, "%ms %ms %ms %ms", &_url, &_arch, &_branch, &_secret);
	if (url)
		*url = _url;
	else
		free(_url);
	if (arch)
		*arch = _arch;
	else
		free(_arch);
	if (branch)
		*branch = _branch;
	else
		free(_branch);
	if (secret)
		*secret = _secret;
	else
		free(_secret);
	return;
	
}


char *dbp_pkglist_source_cache_path(struct DBPPackageList *list, int source_id) {
	char *cache_dir, *full_path;
	char *url, *arch, *branch;
	const gchar *csstr;
	GChecksum *cs;

	cache_dir = dbp_mgr_cache_directory();
	_get_repo_line_info(list->source_id[source_id].url, &url, &arch, &branch, NULL);
	
	cs = g_checksum_new(G_CHECKSUM_MD5);
	g_checksum_update(cs, (const uint8_t *) url, -1);
	csstr = g_checksum_get_string(cs);
	asprintf(&full_path, "%s/%s/%s/%s.pkglist", cache_dir, arch, csstr, branch);
	g_checksum_free(cs);
	free(cache_dir);
	free(url), free(arch), free(branch);
	
	return full_path;
}


/* FIXME: We probably want some error reporting */
void dbp_pkglist_cache_read(struct DBPPackageList *list) {
	int i, lock;
	char *cache_file, *branch;
	struct DBPDesktopFile *df;
	struct stat sbuf;
	
	if ((lock = _obtain_pkg_cache_lock()) < 0)
		return;

	for (i = 0; i < list->source_ids; i++) {
		cache_file = dbp_pkglist_source_cache_path(list, i);
		if (stat(cache_file, &sbuf) < 0)
			list->source_id[i].last_update = 0;
		else
			list->source_id[i].last_update = sbuf.st_mtime;
		if ((df = dbp_desktop_parse_file(cache_file))) {
			_get_repo_line_info(list->source_id[i].url, NULL, NULL, &branch, NULL);
			dbp_pkglist_parse(list, branch, i, df);
			free(branch);
			dbp_desktop_free(df);
		}
		free(cache_file);
	}
	
	dbp_mgr_file_unlock(lock);

	return;
}


void dbp_pkglist_cache_nuke(struct DBPPackageList *list) {
	int i, lock;
	char *path;

	if ((lock = _obtain_pkg_cache_lock()) < 0)
		return;

	for (i = 0; i < list->source_ids; i++) {
		path = dbp_pkglist_source_cache_path(list, i);
		unlink(path);
		free(path);
	}
	
	dbp_mgr_file_unlock(lock);

	return;
}


/* FIXME: We probably want some error reporting ...*/
/* FIXME: We probably shouldn't use the curl easy interface like this, but it works as a prototype.. */
void dbp_pkglist_cache_update_one(struct DBPPackageList *list, int source_id) {
	char *url, *arch, *branch, *secret, *repo_uri = NULL, *cache_file_tmp = NULL, *cache_file, *tmp, *dir;
	char cerr[CURL_ERROR_SIZE];
	CURL *curly;
	FILE *fp;
	
	if (!(curly = curl_easy_init())) {
		fprintf(stderr, "dbp_pkglist_cache_update_one(): Unable to init curl\n");
		return;
	}
	
	cache_file = dbp_pkglist_source_cache_path(list, source_id);
	tmp = strdup(cache_file);
	dir = dirname(tmp);
	if (dbp_mgr_mkdir_recursive(dir, 0700)) {
		free(tmp);
		goto error;
	}

	free(tmp);

	asprintf(&cache_file_tmp, "%s.part", cache_file);
	if (!(fp = fopen(cache_file_tmp, "w"))) {
		goto error;
	}

	_get_repo_line_info(list->source_id[source_id].url, &url, &arch, &branch, &secret);
	asprintf(&repo_uri, "%s/%s/%s/pkglist.gz", url, branch, arch);
	curl_easy_setopt(curly, CURLOPT_URL, repo_uri);
	curl_easy_setopt(curly, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curly, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curly, CURLOPT_ERRORBUFFER, cerr);
	/* TODO: Add secret if present */
	
	if (curl_easy_perform(curly) != CURLE_OK) {
		fprintf(stderr, "CURL error: %s\n", cerr);
		unlink(cache_file_tmp);
		goto error;
	}

	fclose(fp), fp = NULL;
	/* Decompress it */ {
		int i;
		gzFile gzf;
		FILE *fp2;
		char buff[8192];
		unlink(cache_file);
		if (!(fp2 = fopen(cache_file, "wb"))) {
			/* Fuck. */
			fprintf(stderr, "Unable to open target package list cache file '%s'\n", cache_file);
			goto error;
		}

		if (!(gzf = gzopen(cache_file_tmp, "rb"))) {
			// Should not happen
			fclose(fp);
			goto error;
		}

		for (i = gzread(gzf, buff, 8192); i > 0; i = gzread(gzf, buff, 8192))
			fwrite(buff, i, 1, fp2);
		if (i < 0) {
			fprintf(stderr, "decompression error at i=%lli in file %s\n", (long long int) ftell(fp), cache_file_tmp);
			unlink(cache_file);
		}

		gzclose(gzf);
		fclose(fp2);
	}

	error:
	unlink(cache_file_tmp);
	free(repo_uri);
	free(cache_file_tmp);
	free(cache_file);
	if (fp)
		fclose(fp);

}


void dbp_pkglist_cache_update(struct DBPPackageList *list) {
	int i;

	for (i = 0; i < list->source_ids; i++)
		dbp_pkglist_cache_update_one(list, i);
	return;

}
