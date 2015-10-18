/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'dbpd-dbus-client.c' - This file is part of libdbpmgr

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

#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbp.h"
#include "dbpd-types.h"

#include <libintl.h>
#define	_(STRING)	dgettext(DBP_GETTEXT_DOMAIN, STRING)

#define	SEND_MESSAGE(method, arg, retfail)																			\
	GError *error = NULL;																					\
	GVariant *ret;																						\
	int reti = -1;																						\
	if (!(ret = g_dbus_connection_call_sync(conn, DBP_DBUS_DAEMON_PREFIX, DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, method, arg, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error))) {	\
		fprintf(stderr, "Unable to access dbpd: %s\n", error->message);															\
		g_error_free(error);																				\
		return retfail;																					\
	}

static GDBusConnection *conn;

int dbpmgr_server_ping() {
	SEND_MESSAGE("Ping", NULL, -1);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}


int dbpmgr_server_mount(const char *pkg_id, const char *user) {
	SEND_MESSAGE("Mount", g_variant_new("(ss)", pkg_id, user), -1);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_umount(int mount_ref) {
	SEND_MESSAGE("UMount", g_variant_new("(i)", mount_ref), -1);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_mountpoint_get(const char *pkg_id, char **mountpoint) {
	char *mpoint = NULL;
	SEND_MESSAGE("MountPointGet", g_variant_new("(s)", pkg_id), -1);
	g_variant_get(ret, "(is)", &reti, &mpoint);
	*mountpoint = strdup(mpoint);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_register_path(const char *path, char **pkg_id) {
	char *id;
	SEND_MESSAGE("RegisterPath", g_variant_new("(s)", path), -1);
	g_variant_get(ret, "(is)", &reti, &id);
	*pkg_id = strdup(id);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_unregister_path(const char *pkg_id) {
	SEND_MESSAGE("UnregisterPath", g_variant_new("(s)", pkg_id), -1);
	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_id_from_path(const char *path, char **pkg_id) {
	char *id;
	SEND_MESSAGE("IdFromPath", g_variant_new("(s)", path), -1);
	g_variant_get(ret, "(is)", &reti, &id);
	*pkg_id = strdup(id);
	g_variant_unref(ret);
	
	return reti;
}

int dbpmgr_server_path_from_id(const char *pkg_id, char **path) {
	char *path_c;
	SEND_MESSAGE("PathFromId", g_variant_new("(s)", pkg_id), -1);
	g_variant_get(ret, "(is)", &reti, &path_c);
	*path = strdup(path_c);
	g_variant_unref(ret);

	return reti;
}

struct DBPList *dbpmgr_server_package_list() {
	GVariantIter *iter;
	char *path, *id, *desktop;
	struct DBPList *prev = NULL, *this = NULL;
	SEND_MESSAGE("PackageList", NULL, NULL);
	(void) reti;
	g_variant_get(ret, "(a(sss))", &iter);
	while (g_variant_iter_loop(iter, "(sss)", &path, &id, &desktop)) {
		if (!strcmp(id, "!"))
			continue;
		this = malloc(sizeof(*this));
		this->path = strdup(path);
		this->id = strdup(id);
		this->on_desktop = !strcmp(desktop, "desk");
		this->next = prev;
		prev = this;
	}

	g_variant_iter_free(iter);
	g_variant_unref(ret);
	
	return this;
}


void dbpmgr_server_package_list_free(struct DBPList *list) {
	struct DBPList *old;
	for (old = list; list; old = list, list = list->next, free(old)) {
		free(list->path);
		free(list->id);
	}
}


int dbpmgr_server_connect() {
	GError *error = NULL;

	if (!(conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error))) {
		if (!conn) {
			fprintf(stderr, "Unable to connect to DBus system bus: %s\n", error->message);
			g_error_free(error);
			return -1;
		}
	}

	return dbpmgr_server_ping();
}
