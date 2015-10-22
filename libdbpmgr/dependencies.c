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


#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef __NO_DEBIAN_DEPENDS
	#define	LIBDPKG_VOLATILE_API
	#include <dpkg/dpkg.h>
	#include <dpkg/arch.h>
	#include <dpkg/dpkg-db.h>

	static bool dpkg_init = false;
#endif

// Not needed
#include "types.h"

static char *default_arch;

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


void dbpmgr_depend_free(struct DBPDepend *dep) {
	int i;
	if (!dep)
		return;
	free(dep->arch), free(dep->pkg_name);
	for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++)
		free(dep->version[i]);
	free(dep);
}


struct DBPDepend *dbpmgr_depend_parse(const char *package_string) {
	struct DBPDepend *version;
	char *tmp, *ver = NULL, *arch, *pkg_and_arch, *pkg, *next;
	const char *comparisons[DBPMGR_DEPEND_VERSION_CHECKS] = { "<=", ">=", "<", "=", ">" };
	int i;

	#warning Load a default arch
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


#ifndef __NO_DEBIAN_DEPENDS

static void debian_check_do_init() {
	push_error_context();
	dpkg_db_set_dir(NULL);
	modstatdb_open(msdbrw_readonly);
}


static void debian_check_do_free() {
	if (!dpkg_init)
		return;
	modstatdb_done();
	pop_error_context(ehflag_normaltidy);
}

bool dbpmgr_depend_debian_check(const char *package_string) {
	struct pkgset *pkg;
	struct dpkg_arch *arch;
	struct dpkg_version in_ver[DBPMGR_DEPEND_VERSION_CHECKS];
	struct dpkg_error err;
	int ver_match, i;
	struct DBPDepend *dep;


	if (!dpkg_init)
		debian_check_do_init();
	
	if (!package_string)
		return true;
	dep = dbpmgr_depend_parse(package_string);
	for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++) {
		dpkg_version_blank(&in_ver[i]);
		if (dep->version[i])
			if (parseversion(&in_ver[i], dep->version[i], &err)) {
				fprintf(stderr, "Invalid package version in package\n");
				return false;
			}
	}

	pkg = pkg_db_find_set(dep->pkg_name);
	if (!pkg)
		return (dbpmgr_depend_free(dep), false);
	
	for (; pkg; pkg = pkg->next) {
		//fprintf(stderr, "pkg=%s\n", pkgstr);
		if (strcmp(pkg->name, dep->pkg_name))
			continue;
		if (pkg->pkg.status != PKG_STAT_INSTALLED)
			continue;
		for (arch = (struct dpkg_arch *) pkg->pkg.installed.arch; arch; arch = (struct dpkg_arch *) arch->next) {
			if (strcmp(arch->name, "any") && strcmp(arch->name, "all") && strcmp(arch->name, dep->arch) && strcmp(dep->arch, "any"))
				continue;
			//fprintf(stderr, "Arch: %s\n", arch->name);
			for (i = 0; i < DBPMGR_DEPEND_VERSION_CHECKS; i++) {
				if (!dep->version[i])
					continue;
				ver_match = dpkg_version_compare(&pkg->pkg.installed.version, &in_ver[i]);
				if (!dbpmgr_depend_version_result_compare(ver_match, i))
					return dbpmgr_depend_free(dep), false;
			}
		}
	}

	dbpmgr_depend_free(dep);
	return true;
}

#else
bool dbpmgr_depend_debian_check(const char *package_string) {
	// This might be a bit optimistic
	return true;
}

static void debian_check_do_free() {
	return;
}

#endif

void dbpmgr_depend_cleanup() {
	debian_check_do_free();
}
