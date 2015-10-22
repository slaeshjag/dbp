/* deb_dep_check.c - Steven Arnow <s@rdw.se>,  2014 */
#define	LIBDPKG_VOLATILE_API


#include <stdlib.h>
#include <string.h>
#include <dpkg/dpkg.h>
#include <dpkg/arch.h>
#include <dpkg/dpkg-db.h>

#include <dbpmgr/dbpmgr.h>

#if 0
/* Defines for old versions of libdpkg (wheezy) */
#ifndef DPKG_ARCH_NATIVE
#define	DPKG_ARCH_NATIVE arch_native
#endif

#ifndef enum pkgstatus
#define	PKG_STAT_INSTALLED stat_installed
#endif
#endif


void dbp_deb_dep_check_do_init() {
	push_error_context();
	dpkg_db_set_dir(NULL);
	modstatdb_open(msdbrw_readonly);
}


void dbp_deb_dep_check_do_free() {
	modstatdb_done();
	pop_error_context(ehflag_normaltidy);
}


int dbp_deb_dep_check_check_package(const char *pkgname, const char *main_arch) {
	struct pkgset *pkg;
	struct dpkg_arch *arch;
	struct dpkg_version in_ver;
	struct dpkg_error err;
	int flags, ver_match;
	char *pkgstr, *version, *pkg_arch;

	fprintf(stderr, "%s %s\n", pkgname, main_arch);
	dpkg_version_blank(&in_ver);
	if (parseversion(&in_ver, version, &err)) {
		fprintf(stderr, "Invalid package version in package\n");
		return 0;
	}

	pkgstr = strdup(pkgname);
	
	flags = extract_version_arch(pkgstr, &version, &pkg_arch, main_arch);
	pkg = pkg_db_find_set(pkgstr);
	if (!pkg)
		return (free(pkgstr), 0);
	
	for (; pkg; pkg = pkg->next) {
		//fprintf(stderr, "pkg=%s\n", pkgstr);
		if (strcmp(pkg->name, pkgstr))
			continue;
		if (pkg->pkg.status != PKG_STAT_INSTALLED)
			continue;
		for (arch = (struct dpkg_arch *) pkg->pkg.installed.arch; arch; arch = (struct dpkg_arch *) arch->next) {
			if (strcmp(arch->name, "any") && strcmp(arch->name, "all") && strcmp(arch->name, pkg_arch) && strcmp(pkg_arch, "any"))
				continue;
			//fprintf(stderr, "Arch: %s\n", arch->name);
			ver_match = dpkg_version_compare(&pkg->pkg.installed.version, &in_ver);
			//fprintf(stderr, "Version comparison: %i\n", ver_match);
			free(pkgstr);
			/* TODO: Use dpkg built in functions for this when dpkg changes have calmed down */
			if ((flags & 1) && ver_match < 0)
				return 1;
			if ((flags & 2) && !ver_match)
				return 1;
			if ((flags & 4) && ver_match > 0)
				return 1;
			return 0;
		}
	}

	free(pkgstr);
	return 0;
}

/* Nothing's ever going to change, is it? */
