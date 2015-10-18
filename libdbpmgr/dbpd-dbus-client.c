#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>


int dbpmgr_server_connect() {
	GDBusConnection *conn;

	if (!(conn = g_dbus_connection_new_for_address_sync(
}
