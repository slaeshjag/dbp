#ifndef __DBPMGR_DEPENDENCIES_H__
#define __DBPMGR_DEPENDENCIES_H__

#include <dbpmgr/types.h>

struct DBPDepend *dbpmgr_depend_parse(const char *package_string);
/* After doing package dependency checking, run free to unload databases */
void dbpmgr_depend_free();
struct DBPDependDPackage *dbpmgr_depend_debian_next(const char *pkg_name, struct DBPDependDPackageNode *prev);

#endif
