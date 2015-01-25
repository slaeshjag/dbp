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
void comm_dbus_register_thumbnailer();

void comm_dbus_unregister(DBusConnection *dc, void *n) {
	(void) n;
	(void) dc;
	fprintf(dbp_error_log, "Unregister handler called\n");
	return;
}


DBusHandlerResult comm_dbus_msg_handler(DBusConnection *dc, DBusMessage *dm, void *n) {
	DBusMessage *ndm;
	DBusMessageIter iter;
	const char *uri, *flavour;
	uint32_t ret;

	if (!dbus_message_iter_init(dm, &iter)) {
		fprintf(dbp_error_log, "Message has no arguments\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	
	ndm = dbus_message_new_method_return(dm);

	if (dbus_message_is_method_call(dm, DBP_DBUS_THUMB_PREFIX, "Queue")) {
		dbus_message_iter_get_basic(&iter, &uri);
		if (!dbus_message_iter_next(&iter) || !dbus_message_iter_next(&iter)) {
			fprintf(stderr, "Bad queue command\n");
			return DBUS_HANDLER_RESULT_HANDLED;
		}
		dbus_message_iter_get_basic(&iter, &flavour);
		ret = thumb_queue(uri, flavour);
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
	dbus_connection_read_write_dispatch(dc, 100);
	comm_dbus_register_thumbnailer();
	fprintf(stderr, "Thumbnailer registered\n");
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


void comm_dbus_register_thumbnailer() {
	char **uri_sch, **mime, *object;
	DBusMessage *dm;
	DBusPendingCall *pending;

	uri_sch = malloc(sizeof(*uri_sch) * 2);
	mime = malloc(sizeof(*mime) * 2);
	*uri_sch = strdup("file");
	*mime = strdup("application/x-dbp");
	object = strdup(DBP_DBUS_THUMB_OBJECT);
	uri_sch[1] = NULL;
	mime[1] = NULL;

	dm = dbus_message_new_method_call("org.freedesktop.thumbnails.Manager1", "/org/freedesktop/thumbnails/Manager1", "org.freedesktop.thumbnails.Manager1", "Register");
	dbus_message_append_args(dm, DBUS_TYPE_STRING, &object, DBUS_TYPE_INVALID);
	dbus_message_append_args(dm, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &uri_sch, 1, DBUS_TYPE_INVALID);
	dbus_message_append_args(dm, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &mime, 1, DBUS_TYPE_INVALID);
	dbus_connection_send_with_reply(dbus_conn_handle, dm, &pending, 1000);
	if (!pending) {
		fprintf(stderr, "Unable to send registration message\n");
		exit(1);
	}

	dbus_connection_flush(dbus_conn_handle);
	dbus_pending_call_block(pending);
	dbus_message_unref(dm);
	if (!(dm = dbus_pending_call_steal_reply(pending))) {
		dbus_pending_call_unref(pending);
		fprintf(stderr, "No reply from org.freedesktop.thumbnails.Manager1, trying again in a second\n");
		sleep(1);
		return comm_dbus_register_thumbnailer();
	}

	dbus_pending_call_unref(pending);
	dbus_message_unref(dm);

	return;
}
