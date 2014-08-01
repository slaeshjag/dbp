#include <dbus/dbus.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

#include "dbp.h"
#include "util.h"
#include "package.h"

void comm_dbus_unregister(DBusConnection *dc, void *n) {
	fprintf(stderr, "Unregister handler called\n");
	return;
}


DBusHandlerResult comm_dbus_msg_handler(DBusConnection *dc, DBusMessage *dm, void *n) {
	struct package_s *p = n;
	DBusMessage *ndm;
	DBusMessageIter iter;
	const char *arg, *name;
	char *ret, *ret2, *ret3, *mount, *dev;

	if (!dbus_message_iter_init(dm, &iter)) {
		fprintf(stderr, "Message has no arguments\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		fprintf(stderr, "Message has bad argument type\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_get_basic(&iter, &arg);
	dbus_message_iter_next(&iter);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		fprintf(stderr, "Message has bad argument type\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	dbus_message_iter_get_basic(&iter, &name);

	ret = malloc(11);

	ret2 = ret3 = NULL;
	/* Process message */
	if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "Mount")) {
		sprintf(ret, "%i", package_run(p, arg, name));
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "UMount")) {
		sprintf(ret, "%i", package_stop(p, atoi(arg)));
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "MountP")) {
		ret2 = package_mount_get(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "RegisterPath")) {
		util_lookup_mount(name, &mount, &dev);
		sprintf(ret, "%i", package_register_path(p, dev, name, mount, &ret3));
		free(dev), free(mount);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "UnregisterPath")) {
		sprintf(ret, "1");
		package_release_path(p, name);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "IdFromPath")) {
		ret2 = package_id_from_path(p, name);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "GetAppdata")) {
		ret2 = package_appdata_from_id(p, name);
	} else {
		fprintf(stderr, "Bad method call\n");
		goto done;
	}

	ndm = dbus_message_new_method_return(dm);
	if (!ret2)
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret, DBUS_TYPE_INVALID);
	else
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret2, DBUS_TYPE_INVALID);
	if (ret3)
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret3, DBUS_TYPE_INVALID);
	dbus_connection_send(dc, ndm, NULL);
	dbus_connection_flush(dc);

	dbus_message_unref(ndm);
	done:
	free(ret);
	free(ret2);
	free(ret3);

	return DBUS_HANDLER_RESULT_HANDLED;
}


void *comm_dbus_loop(void *n) {
	struct package_s *p = n;
	DBusError err;
	DBusConnection *dc;
	DBusObjectPathVTable vt;

	vt.unregister_function = comm_dbus_unregister;
	vt.message_function = comm_dbus_msg_handler;

	dbus_error_init(&err);
	if (!(dc = dbus_bus_get(DBUS_BUS_SYSTEM, &err))) {
		fprintf(stderr, "Unable to connect to dbus system bus\n");
		exit(-1);
	}

	dbus_bus_request_name(dc, DBP_DBUS_DAEMON_PREFIX, 0, &err);
	if (dbus_error_is_set(&err))
		fprintf(stderr, "unable to name bus: %s\n", err.message);
	if (!dbus_connection_register_object_path(dc, DBP_DBUS_DAEMON_OBJECT, &vt, p))
		fprintf(stderr, "Unable to register object path\n");
	while (dbus_connection_read_write_dispatch (dc, -1));
	pthread_exit(NULL);
}


void comm_dbus_register(struct package_s *p) {
	pthread_t th;

	if (pthread_create(&th, NULL, comm_dbus_loop, p)) {
		fprintf(stderr, "Unable to create dbus listen thread\n");
		exit(-1);
	}
}
