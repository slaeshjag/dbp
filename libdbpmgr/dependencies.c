/*
Copyright (c) 2014, 2015 Steven Arnow <s@rdw.se>
'dependencies.c' - This file is part of libdbpmgr

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

#define	_BSD_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dbpbase/dbpbase.h>
#include "types.h"


static void free_list(struct DBPDependDPackage *list);
static char *default_arch = NULL;

struct DPackageVersion {
	char			*epoch;
	char			*main;
	char			*rev;
};

/* I am so, so  sorry about this */

// This is not very pretty
static char *find_next_version_param(char *ver) {
	int i;

	for (i = 0; ver[i]; i++)
		if (ver[i] == '<' || ver[i] == '=' || ver[i] == '>')
			break;
	if (!i && ver[i])
		return find_next_version_param(ver + 1);
	return ver[i]?&ver[i]:NULL;
}

bool dbpmgr_depend_version_result_compare(int result, enum DBPMgrDependVersionCheck check) {
	if (DBPMGR_DEPEND_VERSION_CHECK_LTEQ == check)
		return result <= 0;
	if (DBPMGR_DEPEND_VERSION_CHECK_GTEQ == check)
		return result >= 0;
	if (DBPMGR_DEPEND_VERSION_CHECK_LT == check)
		return result < 0;
	if (DBPMGR_DEPEND_VERSION_CHECK_EQ == check)
		return result == 0;
	if (DBPMGR_DEPEND_VERSION_CHECK_GT == check)
		return result > 0;
	return 0;
}

struct DBPDepend *dbpmgr_depend_parse(const char *package_string) {
	struct DBPDepend *version;
	char *tmp, *ver = NULL, *arch, *pkg_and_arch, *pkg, *next;
	const char *comparisons[DBPMGR_DEPEND_VERSION_CHECKS] = { "<=", ">=", "<", "=", ">" };
	int i;

	version = calloc(sizeof(*version), 1);
	if (strchr(package_string, '<') || strchr(package_string, '=') || strchr(package_string, '>')) {
		if ((tmp = strchr(package_string, '<')))
			if (ver > tmp || !ver) ver = tmp;
		if ((tmp = strchr(package_string, '=')))
			if (ver > tmp || !ver) ver = tmp;
		if ((tmp = strchr(package_string, '>')))
			if (ver > tmp || !ver) ver = tmp;
	}

	if (ver)
		pkg_and_arch = strndup(package_string, ver - package_string);
	else
		pkg_and_arch = strdup(package_string);
	if ((arch = strchr(pkg_and_arch, ':')))
		pkg = strndup(pkg_and_arch, arch - pkg_and_arch);
	else
		pkg = strdup(pkg_and_arch);
	version->pkg_name = pkg;
	version->arch = arch?strdup(arch + 1):strdup(default_arch?default_arch:"any");	// Eliminate the preceeding :
	free(pkg_and_arch);

	// Ugh. I guess it's time to parse the version now.
	for (next = ver; next; next = find_next_version_param(next)) {
		int len;
		if (find_next_version_param(next))
			len = find_next_version_param(next) - next;
		else
			len = strlen(next);
		
		for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++)
			if (!strncmp(next, comparisons[i], strlen(comparisons[i]))) {
				if (!version->version[i])
					version->version[i] = strndup(next + strlen(comparisons[i]), len - strlen(comparisons[i]));
				break;
			}
	}

	return version;
}

static char char_to_index[128];
static int lookup_size = 0;
static struct DBPDependDPackageNode *debian_root = NULL;

#define	IS_END(c)	(isdigit((c)) || !(c))

/* I have no idea what I'm doing... */
static int compare_substring(char *s1, char *s2) {
	int i;

	for (i = 0; !IS_END(s1[i]) && !IS_END(s2[i]); i++) {
		if (s1[i] == s2[i])
			continue;
		if (s1[i] == '~')
			return -1;
		if (s2[i] == '~')
			return 1;
		
		if (!isalpha(s1[i])) {
			if (!isalpha(s2[i]))
				return s1[i] < s2[i] ? -1 : 1;
			return -1;
		}
		
		if (isalpha(s1[i])) {
			if (isalpha(s2[i]))
				return s1[i] < s2[i] ? -1 : 1;
			return 1;
		}

	}

	if (IS_END(s1[i]) && IS_END(s2[i]))
		return 0;
	if (IS_END(s1[i]))
		return s2[i] != '~' ? -1 : 1;
	if (IS_END(s2[i]))
		return s1[i] != '~' ? 1 : -1;
	/* All cases handled */
	return 0;
}


static struct DPackageVersion version_decode(char *version) {
	struct DPackageVersion pv = { NULL, NULL, NULL };
	char *t;

	if ((t = strchr(version, ':')))
		pv.epoch = strndup(version, t - version), version = t + 1;
	else
		pv.epoch = strdup("0");
	if ((t = strchr(version, '-'))) {
		for (; strchr(t, '-'); t = strchr(t, '-') + 1);
		pv.rev = strdup(t);
		pv.main = strndup(version, t - version - 1);
	} else {
		if (*version)
			pv.main = strdup(version);
		else
			pv.main = strdup("0");
		pv.rev = strdup("0");
	}
	return pv;
}

static void version_free(struct DPackageVersion pv) {
	free(pv.epoch);
	free(pv.main);
	free(pv.rev);
}

static char *find_next_nonumeric(char *s) {
	while (isdigit(*s))
		s++;
	return s;
}

static char *find_next_numeric(char *s) {
	while (!isdigit(*s) && *s)
		s++;
	return s;
}

static int compare_version_string(char *s1, char *s2) {
	int n1, n2;

	while (*s1 && *s2) {
		n1 = n2 = 0;
		sscanf(s1, "%i", &n1);
		sscanf(s2, "%i", &n2);
		if (n1 < n2)
			return -1;
		if (n1 > n2)
			return 1;
		s1 = find_next_nonumeric(s1);
		s2 = find_next_nonumeric(s2);
		if ((n1 = compare_substring(s1, s2)))
			return n1;
		s1 = find_next_numeric(s1);
		s2 = find_next_numeric(s2);
	}

	if (!*s1 && *s2)
		return *s2 == '~' ? 1 : -1;
	if (*s1 && !*s2)
		return *s1 == '~' ? -1 : 1;

	return 0;
}


int dbpmgr_depend_compare_version(char *ver1, char *ver2) {
	struct DPackageVersion v1, v2;
	int i;

	v1 = version_decode(ver1);
	v2 = version_decode(ver2);
	
	if (atoi(v1.epoch) != atoi(v2.epoch))
		return atoi(v1.epoch) < atoi(v2.epoch) ? -1 : 1;
	i = compare_version_string(v1.main, v2.main);
	if (!i)
		i = compare_version_string(v1.rev, v2.rev);
	version_free(v1);
	version_free(v2);
	return i;
}


static void init_conv_table() {
	int i, j;

	memset(char_to_index, 0, 256);
	for (i = 0, j = 1; i <= 'z' - 'a'; i++, j++)
		char_to_index['a' + i] = j;
	for (i = 0; i <= '9' - '0'; i++, j++)
		char_to_index['0' + i] = j;
	char_to_index['+'] = j++;
	char_to_index['-'] = j++;
	char_to_index['.'] = j++;
	lookup_size = j;
}

static char *load_debian_database() {
	FILE *fp;
	int length;	/* If the package database is > 2 GB, the user have bigger problems... */
	char *dbstr;

	if (!(fp = fopen("/var/lib/dpkg/status", "r")))
		return NULL;
	fseek(fp, 0, SEEK_END), length = ftell(fp), rewind(fp);
	if ((dbstr = malloc(length + 1)))
		fread(dbstr, length, 1, fp), dbstr[length] = 0;

	fclose(fp);
	return dbstr;
}

#define	TREE_LOOKUP(c)	((unsigned char )char_to_index[((unsigned char) (c)) & 0x7F])

static struct DBPDependDPackageNode *package_tree_populate(struct DBPDependDPackageNode *root, struct DBPDependDPackage *entry, int name_index) {
	if (!entry)
		return root;
	if (!root)
		root = calloc(sizeof(*root), 1);
	if (!entry->name[name_index])
		return entry->next = root->match, root->match = entry, root;
	if (!root->lookup)
		entry->next = root->list, root->list = entry, root->list_size++;
	else
		return root->lookup[TREE_LOOKUP(entry->name[name_index])] = package_tree_populate(root->lookup[TREE_LOOKUP(entry->name[name_index])], entry, name_index + 1), root;
	if (root->list_size > 25) {	// Arbitrary value for when a list is "big"
		struct DBPDependDPackage *next, *this;
		root->lookup = calloc(sizeof(*(root->lookup)), lookup_size);
		for (this = root->list; this; this = next)
			next = this->next, root->lookup[TREE_LOOKUP(this->name[name_index])] = package_tree_populate(root->lookup[TREE_LOOKUP(this->name[name_index])], this, name_index + 1);
		root->list = NULL, root->list_size = 0;
	}
	return root;
}

#define	IS_KEY(token, key)	(!strncmp(token, key, strlen(key)))
#define	COPY_VALUE(token, key)	(strdup(token + strlen(key)))

static struct DBPDependDPackageNode *build_database() {
	char *db, *tok, *save;
	struct DBPDependDPackageNode *root = NULL;
	struct DBPDependDPackage *this = NULL;
	bool installed = false;

	if (!(db = load_debian_database()))
		return NULL;
	for (save = db, tok = strsep(&save, "\n"); tok; tok = strsep(&save, "\n")) {
		if (!*tok) {
			if (!installed)
				free_list(this);
			else
				root = package_tree_populate(root, this, 0);
			this = NULL, installed = false;
			continue;
		}
		
		if (!this)
			this = calloc(sizeof(*this), 1);
		if (IS_KEY(tok, "Package: "))
			this->name = COPY_VALUE(tok, "Package: ");
		else if (IS_KEY(tok, "Architecture: "))
			this->arch = COPY_VALUE(tok, "Architecture: ");
		else if (IS_KEY(tok, "Version: "))
			this->version = COPY_VALUE(tok, "Version: ");
		else if (IS_KEY(tok, "Status: "))
			installed = !!strstr(tok, "installed");
	}

	if (this)
		root = package_tree_populate(root, this, 0), this = NULL;

	free(db);
	return root;
}

static void free_list(struct DBPDependDPackage *list) {
	struct DBPDependDPackage *next;
	if (!list)
		return;
	next = list->next;
	free(list->name), free(list->arch), free(list->version), free(list);
	free_list(next);
}

static void free_node(struct DBPDependDPackageNode *node) {
	int i;
	if (!node)
		return;
	free_list(node->match);
	if (node->lookup)
		for (i = 0; i < lookup_size; i++)
			free_node(node->lookup[i]);
	free(node->lookup);
	free_list(node->list);
	free(node);
}

static void dbpmgr_depend_debian_init() {
	if (debian_root)
		return;
	init_conv_table();
	debian_root = build_database();
}


void dbpmgr_depend_arch_set(const char *arch) {
	free(default_arch), default_arch = strdup(arch);
}

static struct DBPDependDPackage *find_list(const char *pkg_name, struct DBPDependDPackageNode *node, int name_index) {
	if (!node)
		return NULL;
	if (!pkg_name[name_index])
		return node->match;
	if (!node->lookup)
		return node->list;
	return find_list(pkg_name, node->lookup[TREE_LOOKUP(pkg_name[name_index])], name_index + 1);
}

struct DBPDependDPackage *dbpmgr_depend_debian_next(const char *pkg_name, struct DBPDependDPackage *prev) {
	dbpmgr_depend_debian_init();
	
	if (!prev)
		prev = find_list(pkg_name, debian_root, 0);
	else
		prev = prev->next;
	for (; prev; prev = prev->next)
		if (!strcmp(prev->name, pkg_name))
			return prev;
	return NULL;
}


void dbpmgr_depend_free(struct DBPDepend *dep) {
	int i;

	for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++)
		free(dep->version[i]);
	free(dep->pkg_name);
	free(dep->arch);
	free(dep);
	return;
}


bool dbpmgr_depend_debian_check(const char *package_string) {
	int i, ver_match;
	struct DBPDepend *dep;
	struct DBPDependDPackage *pkg;

	if (!package_string)
		return true;
	dep = dbpmgr_depend_parse(package_string);
	
	for (pkg = dbpmgr_depend_debian_next(dep->pkg_name, NULL); pkg; pkg = dbpmgr_depend_debian_next(dep->pkg_name, pkg)) {
		if (strcmp(pkg->name, dep->pkg_name))
			continue;
		if (strcmp(pkg->arch, "any") && strcmp(pkg->arch, "all") && strcmp(pkg->arch, dep->arch) && strcmp(dep->arch, "any"))
			continue;
		for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++) {
			if (!dep->version[i])
				continue;
			ver_match = dbpmgr_depend_compare_version(pkg->version, dep->version[i]);
			if (!dbpmgr_depend_version_result_compare(ver_match, i))
				return dbpmgr_depend_free(dep), false;
		}
		dbpmgr_depend_free(dep);
		return true;
	}

	dbpmgr_depend_free(dep);
	return false;
}

static struct DBPDependDPackageNode *dbp_root = NULL;
struct DBPList *dbpmgr_server_package_list();                                   
void dbpmgr_server_package_list_free(struct DBPList *list);

static void build_dbp_database() {
	struct DBPList *list, *next;
	struct DBPDependDPackage *this = NULL;

	if (!(list = dbpmgr_server_package_list()))
		return;
	for (next = list; next; next = next->next) {
		this = calloc(sizeof(*this), 1);
		this->version = strdup(next->version);
		this->name = strdup(next->id);
		dbp_root = package_tree_populate(dbp_root, this, 0);
	}

	dbpmgr_server_package_list_free(list);
	return;
}

void dbpmgr_depend_dbp_init() {
	if (!dbp_root)
		build_dbp_database();
}


struct DBPDependDPackage *dbpmgr_depend_dbp_next(const char *pkg_name, struct DBPDependDPackage *prev) {
	dbpmgr_depend_dbp_init();
	
	if (!prev)
		prev = find_list(pkg_name, dbp_root, 0);
	else
		prev = prev->next;
	for (; prev; prev = prev->next)
		if (!strcmp(prev->name, pkg_name))
			return prev;
	return NULL;
}


bool dbpmgr_depend_dbp_check(const char *package_string) {
	struct DBPDepend *dep;
	struct DBPDependDPackage *pkg;
	int i, ver_match;
	dep = dbpmgr_depend_parse(package_string);

	for (pkg = dbpmgr_depend_dbp_next(dep->pkg_name, NULL); pkg; pkg = dbpmgr_depend_dbp_next(dep->pkg_name, pkg)) {
		for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++) {
			if (!dep->version[i])
				continue;
			ver_match = dbpmgr_depend_compare_version(pkg->version, dep->version[i]);
			if (!dbpmgr_depend_version_result_compare(ver_match, i))
				return dbpmgr_depend_free(dep), false;
		}
		dbpmgr_depend_free(dep);
		return true;
	}
	dbpmgr_depend_free(dep);
	return false;
}


static struct DBPDependList create_list(const char *str) {
	struct DBPDependList list;
	char *new;
	list.depend = NULL, list.depends = 0;
	if (!str)
		return list;
	new = strdup(str);
	dbp_config_expand_token(&list.depend, &list.depends, new);
	free(new);

	return list;
}


static void addto_list(struct DBPDependList *list, const char *str) {
	int id = list->depends++;
	list->depend = realloc(list->depend, sizeof(*list->depend) * list->depends);
	list->depend[id] = strdup(str);
	return;
}


static void dbpmgr_depend_delete_list(struct DBPDependList *list) {
	int i;

	for (i = 0; i < list->depends; i++)
		free(list->depend[i]);
	free(list->depend);
}


void dbpmgr_depend_delete_list_ptr(struct DBPDependListList *list) {
	dbpmgr_depend_delete_list(&list->sysonly);
	dbpmgr_depend_delete_list(&list->syspref);
	dbpmgr_depend_delete_list(&list->dbponly);
	dbpmgr_depend_delete_list(&list->dbppref);
	dbpmgr_depend_delete_list(&list->whatevs);
	free(list);
}


void dbpmgr_depend_cleanup() {
	free_node(debian_root), debian_root = NULL;
	free_node(dbp_root), dbp_root = NULL;
}


struct DBPDependListList *dbpmgr_depend_check(struct DBPDesktopFile *meta) {
	char *sysonly, *dbponly, *prefsys, *prefdbp, *whatevs;
	struct DBPDependListList list, missing, *missing_ptr;
	int i;

	sysonly = dbp_desktop_lookup(meta, "Dependency", "deb", "Package Entry");
	dbponly = dbp_desktop_lookup(meta, "Dependency", "dbp", "Package Entry");
	prefsys = dbp_desktop_lookup(meta, "Dependency", "pref_deb", "Package Entry");
	prefdbp = dbp_desktop_lookup(meta, "Dependency", "pref_dbp", "Package Entry");
	whatevs = dbp_desktop_lookup(meta, "Dependency", "", "Package Entry");
	
	list.sysonly = create_list(sysonly);
	list.dbponly = create_list(dbponly);
	list.syspref = create_list(prefsys);
	list.dbppref = create_list(prefdbp);
	list.whatevs = create_list(whatevs);
	memset(&missing, 0, sizeof(missing));

	/* Det här luktar jommpakod... */
	for (i = 0; i < list.sysonly.depends; i++)
		if (!dbpmgr_depend_debian_check(list.sysonly.depend[i]))
			addto_list(&missing.sysonly, list.sysonly.depend[i]);
	for (i = 0; i < list.syspref.depends; i++)
		if (!dbpmgr_depend_debian_check(list.syspref.depend[i]))
			if (!dbpmgr_depend_dbp_check(list.syspref.depend[i]))
				addto_list(&missing.syspref, list.syspref.depend[i]);
	for (i = 0; i < list.dbponly.depends; i++)
		if (!dbpmgr_depend_dbp_check(list.dbponly.depend[i]))
			addto_list(&missing.dbponly, list.dbponly.depend[i]);
	for (i = 0; i < list.dbppref.depends; i++)
		if (!dbpmgr_depend_dbp_check(list.dbppref.depend[i]))
			if (!dbpmgr_depend_debian_check(list.dbppref.depend[i]))
				addto_list(&missing.dbppref, list.dbppref.depend[i]);
	for (i = 0; i < list.whatevs.depends; i++)
		if (!dbpmgr_depend_dbp_check(list.whatevs.depend[i]))
			if (!dbpmgr_depend_debian_check(list.whatevs.depend[i]))
				addto_list(&missing.whatevs, list.whatevs.depend[i]);
	
	dbpmgr_depend_delete_list(&list.sysonly), dbpmgr_depend_delete_list(&list.syspref);
	dbpmgr_depend_delete_list(&list.dbponly), dbpmgr_depend_delete_list(&list.dbppref);
	dbpmgr_depend_delete_list(&list.whatevs);
	dbpmgr_depend_cleanup();
	
	missing_ptr = malloc(sizeof(*missing_ptr));
	*missing_ptr = missing;

	return missing_ptr;
}



