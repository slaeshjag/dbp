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
#include <pthread.h>

#include "dbp.h"
#include "types.h"

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
static GMainLoop *loop;
pthread_t server_signal_th;

static gint name_watcher = 0;
static gint subscription = 0;
static void *user_data;
static void (*user_signal_handler)(const char *signal, const char *value, void *data);

int dbpmgr_server_ping() {
	SEND_MESSAGE("Ping", NULL, DBP_ERROR_NO_REPLY);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}


int dbpmgr_server_mount(const char *pkg_id, const char *user) {
	SEND_MESSAGE("Mount", g_variant_new("(ss)", pkg_id, user), DBP_ERROR_NO_REPLY);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_umount(int mount_ref) {
	SEND_MESSAGE("UMount", g_variant_new("(i)", mount_ref), DBP_ERROR_NO_REPLY);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_mountpoint_get(const char *pkg_id, char **mountpoint) {
	char *mpoint = NULL;
	SEND_MESSAGE("MountPointGet", g_variant_new("(s)", pkg_id), DBP_ERROR_NO_REPLY);
	g_variant_get(ret, "(is)", &reti, &mpoint);
	*mountpoint = strdup(mpoint);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_register_path(const char *path, char **pkg_id) {
	char *id;
	SEND_MESSAGE("RegisterPath", g_variant_new("(s)", path), DBP_ERROR_NO_REPLY);
	g_variant_get(ret, "(is)", &reti, &id);
	*pkg_id = strdup(id);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_unregister_path(const char *pkg_id) {
	SEND_MESSAGE("UnregisterPath", g_variant_new("(s)", pkg_id), DBP_ERROR_NO_REPLY);
	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_id_from_path(const char *path, char **pkg_id) {
	char *id;
	SEND_MESSAGE("IdFromPath", g_variant_new("(s)", path), DBP_ERROR_NO_REPLY);
	g_variant_get(ret, "(is)", &reti, &id);
	*pkg_id = strdup(id);
	g_variant_unref(ret);
	
	return reti;
}

int dbpmgr_server_path_from_id(const char *pkg_id, char **path) {
	char *path_c;
	SEND_MESSAGE("PathFromId", g_variant_new("(s)", pkg_id), DBP_ERROR_NO_REPLY);
	g_variant_get(ret, "(is)", &reti, &path_c);
	*path = strdup(path_c);
	g_variant_unref(ret);

	return reti;
}

struct DBPList *dbpmgr_server_package_list() {
	GVariantIter *iter;
	char *path, *id, *desktop, *version;
	struct DBPList *prev = NULL, *this = NULL;
	SEND_MESSAGE("PackageList", NULL, NULL);
	(void) reti;
	g_variant_get(ret, "(a(ssss))", &iter);
	for (path = id = desktop = version = NULL; g_variant_iter_loop(iter, "(ssss)", &path, &id, &desktop, &version); path = id = desktop = version = NULL) {
		if (!strcmp(id, "!"))
			continue;
		this = malloc(sizeof(*this));
		this->path = strdup(path);
		this->id = strdup(id);
		this->version = strdup(version);
		this->on_desktop = !strcmp(desktop, "desk");
		this->next = prev;
		prev = this;
	}

	g_variant_iter_free(iter);
	g_variant_unref(ret);
	
	return this;
}

void dbpmgr_server_package_list_free_one(struct DBPList *list) {
	free(list->path);
	free(list->id);
	free(list->version);
	free(list);
}

void dbpmgr_server_package_list_free(struct DBPList *list) {
	struct DBPList *old;
	for (old = list; list; old = list, list = list->next, dbpmgr_server_package_list_free_one(old));
}


static void signal_act(GDBusConnection *dconn, const gchar *sender, const gchar *object, const gchar *interface, const gchar *signal, GVariant *param, gpointer udata) {
	(void) dconn; (void) sender; (void) object; (void) interface; (void) udata;
	char *arg = NULL;

	g_variant_get(param, "(s)", &arg);
	if (arg)
		user_signal_handler(signal, arg, user_data);
}

static void name_known(GDBusConnection *dconn, const gchar *name, const gchar *name_owner, gpointer user_data) {
	(void) name; (void) user_data;
	subscription = g_dbus_connection_signal_subscribe(dconn, name_owner, DBP_DBUS_DAEMON_PREFIX, NULL, DBP_DBUS_DAEMON_OBJECT, NULL, G_DBUS_SIGNAL_FLAGS_NONE, signal_act, NULL, NULL);
}

static void name_vanished(GDBusConnection *dconn, const gchar *name, gpointer user_data) {
	(void) name; (void) user_data;
	g_dbus_connection_signal_unsubscribe(dconn, subscription), subscription = 0;
}

static void *handle_main_loop(void *data) {
	(void) data;
	g_main_loop_run(loop);
	pthread_exit(NULL);
	
}

void dbpmgr_server_signal_listen_sync(void (*signal_handler)(const char *signal, const char *value, void *data), void *data) {
	user_data = data;
	user_signal_handler = signal_handler;

	name_watcher = g_bus_watch_name(G_BUS_TYPE_SYSTEM, DBP_DBUS_DAEMON_PREFIX, G_BUS_NAME_WATCHER_FLAGS_NONE, name_known, name_vanished, NULL, NULL);
	loop = g_main_loop_new(NULL, false);
	g_main_loop_run(loop);
}

void dbpmgr_server_signal_listen(void (*signal_handler)(const char *signal, const char *value, void *data), void *data) {
	user_data = data;
	user_signal_handler = signal_handler;

	name_watcher = g_bus_watch_name(G_BUS_TYPE_SYSTEM, DBP_DBUS_DAEMON_PREFIX, G_BUS_NAME_WATCHER_FLAGS_NONE, name_known, name_vanished, NULL, NULL);
	loop = g_main_loop_new(NULL, false);
	pthread_create(&server_signal_th, NULL, handle_main_loop, NULL);
}


int dbpmgr_server_connect() {
	GError *error = NULL;

	if (!(conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error))) {
		if (!conn) {
			fprintf(stderr, "Unable to connect to DBus system bus: %s\n", error->message);
			g_error_free(error);
			return DBP_ERROR_NO_REPLY;
		}
	}

	return dbpmgr_server_ping();
}
