/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'dbpd-dbus-client.h' - This file is part of libdbpmgr

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

#ifndef __DBPMGR_DBPD_DBUS_CLIENT_H__
#define __DBPMGR_DBPD_DBUS_CLIENT_H__

#include <dbpmgr/types.h>
#include <pthread.h>

/* Connect must be called before any dbpd calls can be made */
int dbpmgr_server_connect();

int dbpmgr_server_ping();
int dbpmgr_server_mount(const char *pkg_id, const char *user);
int dbpmgr_server_umount(int mount_ref);
int dbpmgr_server_mountpoint_get(const char *pkg_id, char **mountpoint);
int dbpmgr_server_register_path(const char *path, char **pkg_id);
int dbpmgr_server_unregister_path(const char *pkg_id);
int dbpmgr_server_id_from_path(const char *path, char **pkg_id);
int dbpmgr_server_path_from_id(const char *pkg_id, char **path);
struct DBPList *dbpmgr_server_package_list();
void dbpmgr_server_package_list_free(struct DBPList *list);
void dbpmgr_server_package_list_free_one(struct DBPList *list);


/* When using async thread listening, this thread runs the dbus main loop */
extern pthread_t server_signal_th;

void dbpmgr_server_signal_listen(void (*signal_handler)(const char *signal, const char *value, void *data), void *data);
void dbpmgr_server_signal_listen_sync(void (*signal_handler)(const char *signal, const char *value, void *data), void *data);

#endif
