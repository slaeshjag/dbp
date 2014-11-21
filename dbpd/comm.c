#include <dbus/dbus.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbp.h"
#include "util.h"
#include "package.h"

DBusConnection *dbus_conn_handle;

void comm_dbus_unregister(DBusConnection *dc, void *n) {
	(void) n;
	(void) dc;
	fprintf(dbp_error_log, "Unregister handler called\n");
	return;
}


/* I am not proud of this function. Sorry. */
DBusHandlerResult comm_dbus_msg_handler(DBusConnection *dc, DBusMessage *dm, void *n) {
	struct package_s *p = n;
	DBusMessage *ndm;
	DBusMessageIter iter;
	const char *arg, *name;
	char *ret, *ret2, *ret3, *mount, *dev, **arr, **new_arr = NULL;
	int i;

	if (!dbus_message_iter_init(dm, &iter)) {
		fprintf(dbp_error_log, "Message has no arguments\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		fprintf(dbp_error_log, "Message has bad argument type\n");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_message_iter_get_basic(&iter, &arg);
	dbus_message_iter_next(&iter);
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
	//	fprintf(dbp_error_log, "Message has bad argument type\n");
		//return DBUS_HANDLER_RESULT_HANDLED;
		name = NULL;
	} else
		dbus_message_iter_get_basic(&iter, &name);

	ret = malloc(11);

	ret2 = ret3 = NULL;
	/* Process message */
	if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "Mount")) {
		if (!name) return DBUS_HANDLER_RESULT_HANDLED;
		sprintf(ret, "%i", package_run(p, arg, name));
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "UMount")) {
		sprintf(ret, "%i", package_stop(p, atoi(arg)));
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "MountPointGet")) {
		ret2 = package_mount_get(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "RegisterPath")) {
		util_lookup_mount(arg, &mount, &dev);
		sprintf(ret, "%i", package_register_path(p, dev, arg, mount, &ret3));
		free(dev), free(mount);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "UnregisterPath")) {
		sprintf(ret, "1");
		package_release_path(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "IdFromPath")) {
		ret2 = package_id_from_path(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "GetAppdata")) {	/* TODO: Remove */
		ret2 = package_appdata_from_id(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "GetPkgDep")) {	/* TODO: Remove */
		package_deps_from_id(p, arg, &ret2, &ret3);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "PathFromId")) {
		ret2 = package_path_from_id(p, arg);
	} else if (dbus_message_is_method_call(dm, DBP_DBUS_DAEMON_PREFIX, "PackageList")) {
		ndm = dbus_message_new_method_return(dm);
		
		pthread_mutex_lock(&p->mutex);
		new_arr = malloc(sizeof(char *) * p->entries * 3);
		for (i = 0; i < p->entries; i++) {
			new_arr[i*3] = p->entry[i].path;
			new_arr[i*3+1] = p->entry[i].id;
			new_arr[i*3+2] = p->entry[i].desktop;
		}
	
		dbus_message_append_args(ndm, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &new_arr, (p->entries * 3), DBUS_TYPE_INVALID);
		free(new_arr);
		pthread_mutex_unlock(&p->mutex);

		goto send_message;
	} else {
		fprintf(dbp_error_log, "Bad method call %s\n", dbus_message_get_member(dm));
		goto done;
	}

	ndm = dbus_message_new_method_return(dm);
	if (!ret2)
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret, DBUS_TYPE_INVALID);
	else
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret2, DBUS_TYPE_INVALID);
	if (ret3)
		dbus_message_append_args(ndm, DBUS_TYPE_STRING, &ret3, DBUS_TYPE_INVALID);

	send_message:
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


	dbus_threads_init_default();
	vt.unregister_function = comm_dbus_unregister;
	vt.message_function = comm_dbus_msg_handler;

	dbus_error_init(&err);
	if (!(dc = dbus_bus_get(DBUS_BUS_SYSTEM, &err))) {
		fprintf(dbp_error_log, "Unable to connect to dbus system bus\n");
		exit(-1);
	}

	dbus_bus_request_name(dc, DBP_DBUS_DAEMON_PREFIX, 0, &err);
	if (dbus_error_is_set(&err))
		fprintf(dbp_error_log, "unable to name bus: %s\n", err.message);
	if (!dbus_connection_register_object_path(dc, DBP_DBUS_DAEMON_OBJECT, &vt, p))
		fprintf(dbp_error_log, "Unable to register object path\n");
	dbus_conn_handle = dc;
	while (dbus_connection_read_write_dispatch (dc, 500));
	fprintf(dbp_error_log, "dbus exit\n");
	pthread_exit(NULL);
}


void comm_dbus_register(struct package_s *p) {
	pthread_t th;

	if (pthread_create(&th, NULL, comm_dbus_loop, p)) {
		fprintf(dbp_error_log, "Unable to create dbus listen thread\n");
		exit(-1);
	}
}


void comm_dbus_announce(const char *name, const char *sig_name) {
	DBusMessage *sig;
	char *id;
	
	if (!dbus_conn_handle)
		return;
	sig = dbus_message_new_signal(DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, sig_name);
	id = strdup(name);
	dbus_message_append_args(sig, DBUS_TYPE_STRING, &id, DBUS_TYPE_INVALID);
	
	dbus_connection_send(dbus_conn_handle, sig, NULL);
	dbus_message_unref(sig);

	free(id);
	return;
}

void comm_dbus_announce_new_meta(const char *id) {
	return comm_dbus_announce(id, "NewMeta");
}

void comm_dbus_announce_rem_meta(const char *id) {
	return comm_dbus_announce(id, "RemoveMeta");
}

void comm_dbus_announce_new_package(const char *id) {
	return comm_dbus_announce(id, "NewPackage");
}

void comm_dbus_announce_rem_package(const char *id) {
	return comm_dbus_announce(id, "RemovePackage");
}
