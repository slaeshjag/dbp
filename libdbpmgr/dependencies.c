/* deb_dep_check.c - Steven Arnow <s@rdw.se>,  2014 */
#define	LIBDPKG_VOLATILE_API


#include <stdlib.h>
#include <string.h>
#include <dpkg/dpkg.h>
#include <dpkg/arch.h>
#include <dpkg/dpkg-db.h>
#include "types.h"

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


struct DBPDepend *dbpmgr_depend_parse(const char *package_string) {
	struct DBPDepend *version;
	char *tmp, *ver = NULL, *arch, *pkg_and_arch, *pkg, *next;

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
	version->arch = arch?strdup(arch + 1):NULL;	// Eliminate the preceeding :
	free(pkg_and_arch);

	// Ugh. I guess it's time to parse the version now.
	for (next = ver; next; next = find_next_version_param(next)) {
		int len;
		if (find_next_version_param(next))
			len = find_next_version_param(next) - next;
		else
			len = strlen(next);
		
		if (!strncmp(next, "<=", 2)) {
			if (!version->version.lteq)
				version->version.lteq = strndup(next + 2, len - 2);
		} else if (!strncmp(next, "<", 1)) {
			if (!version->version.lt)
				version->version.lt = strndup(next + 1, len - 1);
		} else if (!strncmp(next, "=", 1)) {
			if (!version->version.eq)
				version->version.eq = strndup(next + 1, len - 1);
		} else if (!strncmp(next, ">=", 2)) {
			if (!version->version.gteq)
				version->version.gteq = strndup(next + 2, len - 2);
		} else if (!strncmp(next, ">", 1)) {
			if (!version->version.gt)
				version->version.gt = strndup(next + 1, len - 1);
		}
	}

	return version;
}
