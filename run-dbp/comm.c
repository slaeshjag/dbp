#include <dbus/dbus.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dbp.h"

/* Srsly dbus, I have no idea of what I'm doing */
DBusConnection *dc;


#define	COMM_DBUS_MSG_EMIT(cmd, arg1, arg2)				\
	dm = dbus_message_new_method_call(DBP_DBUS_DAEMON_PREFIX,	\
	    DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, cmd);	\
	if (!dm)							\
		fprintf(stderr, "unable to create message\n");		\
	dbus_message_append_args(dm, DBUS_TYPE_STRING, &arg1, DBUS_TYPE_STRING, &arg2, DBUS_TYPE_INVALID);	\
	dbus_connection_send_with_reply(dc, dm, &pending, -1);		\
	if (!pending)							\
		fprintf(stderr, "unable to send message\n");		\
	dbus_connection_flush(dc);					\
	dbus_pending_call_block(pending);				\
	dbus_message_unref(dm);						\
									\
	if (!(dm = dbus_pending_call_steal_reply(pending))) {		\
		dbus_pending_call_unref(pending);			\
		return DBP_ERROR_NO_REPLY;				\
	}								\
									\
	dbus_pending_call_unref(pending);				\
	if (!dbus_message_iter_init(dm, &iter))				\
		return DBP_ERROR_INTERNAL_MSG;				\
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {	\
		dbus_message_unref(dm);					\
		return DBP_ERROR_INTERNAL_MSG;				\
	}


void comm_dbus_init() {
	DBusError err;
	
	dbus_error_init(&err);
	if (!(dc = dbus_bus_get(DBUS_BUS_SYSTEM, &err))) {
		dbus_error_free(&err);
		return;
	}
	
	dbus_error_free(&err);

	return;
}

/* Nomnom, copypasta! */
int comm_dbus_request_mountp(const char *pkg_id, char **reply) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;
	
	*reply = NULL;
	COMM_DBUS_MSG_EMIT("MountP", pkg_id, pkg_id);

	dbus_message_iter_get_basic(&iter, &retc);
	*reply = strdup(retc);
	dbus_message_unref(dm);

	if (!strcmp(*reply, "NULL"))
		return DBP_ERROR_BAD_PKG_ID;

	return 0;
}


int comm_dbus_register_path(const char *path, char **id) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;
	int ret;

	COMM_DBUS_MSG_EMIT("RegisterPath", path, path);

	dbus_message_iter_get_basic(&iter, &retc);
	ret = atoi(retc);
	dbus_message_iter_next(&iter);
	
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		dbus_message_unref(dm);
		return DBP_ERROR_INTERNAL_MSG;
	}

	dbus_message_iter_get_basic(&iter, &retc);
	*id = strdup(retc);
	dbus_message_unref(dm);

	return ret;
}


int comm_dbus_unregister_path(const char *path) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;
	int ret;

	COMM_DBUS_MSG_EMIT("UnregisterPath", path, path);

	dbus_message_iter_get_basic(&iter, &retc);
	ret = atoi(retc);
	dbus_message_unref(dm);

	return ret;
}
	


int comm_dbus_request_mount(const char *pkg_id, const char *user) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;
	int ret;

	COMM_DBUS_MSG_EMIT("Mount", pkg_id, user);

	dbus_message_iter_get_basic(&iter, &retc);
	ret = atoi(retc);
	dbus_message_unref(dm);

	return ret;
}
	   

int comm_dbus_get_id_from_path(const char *path, char **rets) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;

	COMM_DBUS_MSG_EMIT("IdFromPath", path, path);

	dbus_message_iter_get_basic(&iter, &retc);
	*rets = strdup(retc);
	dbus_message_unref(dm);

	return 1;
}


int comm_dbus_get_appdata(const char *id, char **rets) {
	DBusMessage *dm;
	DBusPendingCall *pending;
	DBusMessageIter iter;
	const char *retc;

	COMM_DBUS_MSG_EMIT("GetAppdata", id, id);

	dbus_message_iter_get_basic(&iter, &retc);
	*rets = strdup(retc);
	dbus_message_unref(dm);

	return !(!strcmp(id, "!"));
}


void comm_dbus_request_umount(int run_id) {
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
	dbus_message_append_args(dm, DBUS_TYPE_STRING, &argc, DBUS_TYPE_INVALID);
	dbus_connection_send_with_reply(dc, dm, &pending, -1);
	if (!pending)
		fprintf(stderr, "unable to send message\n");
	dbus_connection_flush(dc);
	dbus_pending_call_block(pending);
	dbus_message_unref(dm);

	return;
}
