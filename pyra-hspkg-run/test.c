#include <dbus/dbus.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "dbp.h"

/* Srsly dbus, I have no idea of what I'm doing */
DBusConnection *dc;

int test_dbus_request_mount(const char *pkg_id) {
	DBusError err;
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;
	int ret;

	dbus_error_init(&err);
	if (!(dc = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
		dbus_error_free(&err);
		return -1;
	}
	
	dbus_error_free(&err);
	dm = dbus_message_new_method_call(DBP_DBUS_DAEMON_PREFIX,
	    DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, "Mount");
	if (!dm)
		fprintf(stderr, "unable to create message\n");
	dbus_message_append_args(dm, DBUS_TYPE_STRING, &pkg_id, DBUS_TYPE_INVALID);
	dbus_connection_send_with_reply(dc, dm, &pending, -1);
	if (!pending)
		fprintf(stderr, "unable to send message\n");
	dbus_connection_flush(dc);
	dbus_pending_call_block(pending);
	dbus_message_unref(dm);

	if (!(dm = dbus_pending_call_steal_reply(pending))) {
		dbus_pending_call_unref(pending);
		return DBP_ERROR_NO_REPLY;
	}

	dbus_pending_call_unref(pending);
	if (!dbus_message_iter_init(dm, &iter))
		return DBP_ERROR_INTERNAL_MSG;
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		dbus_message_unref(dm);
		return DBP_ERROR_INTERNAL_MSG;
	}

	dbus_message_iter_get_basic(&iter, &retc);
	ret = atoi(retc);
	dbus_message_unref(dm);

	return ret;
}
	   

void test_dbus_request_umount(int run_id) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	char *argc;
	
	argc = malloc(11);
	sprintf(argc, "%i", run_id);
	dm = dbus_message_new_method_call(DBP_DBUS_DAEMON_PREFIX,
	    DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, "UMount");
	if (!dm)
		fprintf(stderr, "unable to create message\n");
	dbus_message_append_args(dm, DBUS_TYPE_STRING, &argc, DBUS_TYPE_INVALID);
	dbus_connection_send_with_reply(dc, dm, &pending, -1);
	if (!pending)
		fprintf(stderr, "unable to send message\n");
	dbus_connection_flush(dc);
	dbus_pending_call_block(pending);
	dbus_message_unref(dm);

	return;
}


int main(int argc, char **argv) {
	test_dbus_request_umount(test_dbus_request_mount("arne"));
	return 0;
}
