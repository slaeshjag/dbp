#include <dbus/dbus.h>
#include <pthread.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbp.h"
#include "thumb_queue.h"
#include "util.h"

DBusConnection *dbus_conn_handle;

void comm_dbus_unregister(DBusConnection *dc, void *n) {
	(void) n;
	(void) dc;
	fprintf(dbp_error_log, "Unregister handler called\n");
	return;
}


DBusHandlerResult comm_dbus_msg_handler(DBusConnection *dc, DBusMessage *dm, void *n) {
	DBusMessage *ndm;
	DBusMessageIter iter;
	const char *uri;
	uint32_t ret;

	if (!dbus_message_iter_init(dm, &iter)) {
		fprintf(dbp_error_log, "Message has no arguments\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	
	ndm = dbus_message_new_method_return(dm);

	if (dbus_message_is_method_call(dm, DBP_DBUS_THUMB_PREFIX, "Queue")) {
		dbus_message_iter_get_basic(&iter, &uri);
		ret = thumb_queue(uri);
		dbus_message_append_args(ndm, DBUS_TYPE_UINT32, &ret, DBUS_TYPE_INVALID);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_THUMB_PREFIX, "Dequeue")) {
		fprintf(stderr, "STUB: %s/Dequeue()\n", DBP_DBUS_THUMB_OBJECT);
	} else {
		fprintf(stderr, "Unhandled method call\n");
	}
	
	dbus_connection_send(dc, ndm, NULL);
	dbus_connection_flush(dc);

	dbus_message_unref(ndm);
	return DBUS_HANDLER_RESULT_HANDLED;
}


void *comm_dbus_loop(void *n) {
	struct package_s *p = n;
	DBusError err;
	DBusConnection *dc;
	DBusObjectPathVTable vt;


	dbus_threads_init_default();
	vt.unregister_function = comm_dbus_unregister;
	vt.message_function = comm_dbus_msg_handler;

	dbus_error_init(&err);
	if (!(dc = dbus_bus_get(DBUS_BUS_SESSION, &err))) {
		fprintf(dbp_error_log, "Unable to connect to dbus session bus\n");
		exit(-1);
	}

	dbus_bus_request_name(dc, DBP_DBUS_THUMB_PREFIX, 0, &err);
	if (dbus_error_is_set(&err))
		fprintf(dbp_error_log, "unable to name bus: %s\n", err.message);
	if (!dbus_connection_register_object_path(dc, DBP_DBUS_THUMB_OBJECT, &vt, p))
		fprintf(dbp_error_log, "Unable to register object path\n");
	dbus_conn_handle = dc;
	while (dbus_connection_read_write_dispatch(dc, 100));

	fprintf(dbp_error_log, "dbus exit\n");
	return NULL;
}

void comm_dbus_announce(const char *name, const char *sig_name) {
	DBusMessage *sig;
	char *id;
	
	if (!dbus_conn_handle)
		return;
	sig = dbus_message_new_signal(DBP_DBUS_THUMB_OBJECT, DBP_DBUS_THUMB_PREFIX, sig_name);
	id = strdup(name);
	dbus_message_append_args(sig, DBUS_TYPE_STRING, &id, DBUS_TYPE_INVALID);
	
	dbus_connection_send(dbus_conn_handle, sig, NULL);
	dbus_message_unref(sig);

	free(id);
	return;
}


void comm_dbus_announce_started(uint32_t handle) {
	DBusMessage *sig;
	
	sig = dbus_message_new_signal(DBP_DBUS_THUMB_OBJECT, DBP_DBUS_THUMB_PREFIX, "Started");
	dbus_message_append_args(sig, DBUS_TYPE_UINT32, &handle, DBUS_TYPE_INVALID);
	dbus_message_unref(sig);
}

void comm_dbus_announce_finished(uint32_t handle) {
	DBusMessage *sig;
	
	sig = dbus_message_new_signal(DBP_DBUS_THUMB_OBJECT, DBP_DBUS_THUMB_PREFIX, "Finished");
	dbus_message_append_args(sig, DBUS_TYPE_UINT32, &handle, DBUS_TYPE_INVALID);
	dbus_message_unref(sig);
}

void comm_dbus_announce_ready(uint32_t handle, const char *uri) {
	DBusMessage *sig;
	char *uric;
	
	uric = strdup(uri);
	sig = dbus_message_new_signal(DBP_DBUS_THUMB_OBJECT, DBP_DBUS_THUMB_PREFIX, "Ready");
	dbus_message_append_args(sig, DBUS_TYPE_UINT32, &handle, DBUS_TYPE_INVALID);
	dbus_message_append_args(sig, DBUS_TYPE_STRING, &uric, DBUS_TYPE_INVALID);
	dbus_connection_send(dbus_conn_handle, sig, NULL);

	dbus_message_unref(sig);
	free(uric);
}

void comm_dbus_announce_error(uint32_t handle, const char *uri, enum ThumbError err, const char *msg) {
	DBusMessage *sig;
	char *uric, *msgc;
	
	uric = strdup(uri);
	msg = strdup(msg);
	sig = dbus_message_new_signal(DBP_DBUS_THUMB_OBJECT, DBP_DBUS_THUMB_PREFIX, "Error");
	dbus_message_append_args(sig, DBUS_TYPE_UINT32, &handle, DBUS_TYPE_INVALID);
	dbus_message_append_args(sig, DBUS_TYPE_STRING, &uric, DBUS_TYPE_INVALID);
	dbus_message_append_args(sig, DBUS_TYPE_INT32, &err, DBUS_TYPE_INVALID);
	dbus_message_append_args(sig, DBUS_TYPE_STRING, &msgc, DBUS_TYPE_INVALID);

	dbus_connection_send(dbus_conn_handle, sig, NULL);
	dbus_message_unref(sig);
	free(uric);
	free(msgc);
}

