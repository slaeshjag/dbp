/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'dbpmgr.h' - This file is part of libdbpmgr

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

#ifndef __DBPMGR_DBPMGR_H__
#define __DBPMGR_DBPMGR_H__

#include <dbpmgr/error.h>
#include <dbpmgr/types.h>
#include <dbpmgr/dbpd-dbus-client.h>
#include <dbpmgr/dependencies.h>

char *dbp_mgr_cache_directory();
char *dbp_mgr_config_directory();

int dbp_mgr_file_lock(const char *lockfile);
int dbp_mgr_file_unlock(int fd);

int dbp_mgr_mkdir_recursive(const char *path, int mode);

#endif
