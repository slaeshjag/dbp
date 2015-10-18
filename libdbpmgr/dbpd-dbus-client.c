#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbp.h"
#include "dbpd-types.h"

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

int dbpmgr_server_umount(const char *mount_ref) {
	SEND_MESSAGE("UMount", g_variant_new("(s)", mount_ref), -1);

	g_variant_get(ret, "(i)", &reti);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_mountpoint_get(const char *pkg_id, char **mountpoint) {
	char *mpoint = NULL;
	SEND_MESSAGE("MountPointGet", g_variant_new("(s)", pkg_id), -1);
	g_variant_get(ret, "(i)s", &reti, &mpoint);
	*mountpoint = strdup(mpoint);
	g_variant_unref(ret);

	return reti;
}

int dbpmgr_server_register_path(const char *path, char **pkg_id) {
	char *id;
	SEND_MESSAGE("RegisterPath", g_variant_new("(s)", path), -1);
	g_variant_get(ret, "(i)s", &reti, &id);
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
	g_variant_get(ret, "(i)s", &reti, &id);
	*pkg_id = strdup(id);
	g_variant_unref(ret);
	
	return reti;
}

int dbpmgr_server_path_from_id(const char *pkg_id, char **path) {
	char *path_c;
	SEND_MESSAGE("PathFromId", g_variant_new("(s)", pkg_id), -1);
	g_variant_get(ret, "(i)s", &reti, &path_c);
	*path = strdup(path_c);
	g_variant_unref(ret);

	return reti;
}

struct DBPList *dbpmgr_server_package_list() {
	GVariantIter *path_iter, *id_iter, *desktop_iter;
	char *path, *id;
	int desktop;
	struct DBPList *prev = NULL, *this = NULL;
	SEND_MESSAGE("PackageList", NULL, NULL);
	g_variant_get(ret, "asasai", &path_iter, &id_iter, &desktop_iter);
	while (g_variant_iter_loop(path_iter, "s", &path) && g_variant_iter_loop(id_iter, "s", &id) && g_variant_iter_loop(desktop_iter, "i", &desktop)) {
		this = malloc(sizeof(*this));
		this->next = prev;
		this->path = strdup(path); this->id = strdup(id), this->on_desktop = desktop;
		prev = this;
	}

	g_variant_iter_free(path_iter), g_variant_iter_free(id_iter), g_variant_iter_free(desktop_iter);
	g_variant_unref(ret);
	
	return this;
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


int main(int argc, char **argv) {
	dbpmgr_server_connect();
	return 0;
}
