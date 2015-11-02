/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'dependencies.h' - This file is part of libdbpmgr

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

#ifndef __DBPMGR_DEPENDENCIES_H__
#define __DBPMGR_DEPENDENCIES_H__

#include <dbpbase/dbpbase.h>
#include <dbpmgr/types.h>
#include <stdbool.h>

bool dbpmgr_depend_debian_check(const char *package_string);
struct DBPDepend *dbpmgr_depend_parse(const char *package_string);
void dbpmgr_depend_free(struct DBPDepend *dep);
void dbpmgr_depend_version_result_compare(int result, enum DBPMgrDependVersionCheck check);
struct DBPDependListList *dbpmgr_depend_check(struct DBPDesktopFile *meta);

/* After doing package dependency checking, run free to unload databases */
void dbpmgr_depend_cleanup();
struct DBPDependDPackage *dbpmgr_depend_debian_next(const char *pkg_name, struct DBPDependDPackage *prev);

void dbpmgr_depend_delete_list_ptr(struct DBPDependListList *list);

#endif
