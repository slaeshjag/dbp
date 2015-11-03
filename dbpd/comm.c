#include <gio/gio.h>
#include <pthread.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "dbp.h"
#include "util.h"
#include "package.h"

static GMainLoop *loop = NULL;
static GDBusConnection *dconn = NULL;
static struct package_s *p;
bool daemon_exit_error = false;
bool daemon_exited = false;
static GDBusNodeInfo *introspection_data = NULL;
static gchar *introspection_xml = NULL;

#define	VALIDATE_NOT_NULL(p) if (!(p)) {									\
		g_dbus_method_invocation_return_value(inv, g_variant_new("(%i)", DBP_ERROR_NOTVALID));	\
		return;												\
	}


/* This amount of arguments just makes me think of FailAPI32... */
static void vtab_method_call(GDBusConnection *conn, const gchar *sender, const gchar *object_path, const gchar *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *inv, gpointer user_data) {
	(void) conn; (void) *sender; (void) object_path; (void) interface_name; (void) user_data;
	gchar *arg1 = NULL, *arg2 = NULL;
	gint argi = -1;


	if (!g_strcmp0(method_name, "Mount")) {
		g_variant_get(parameters, "(ss)", &arg1, &arg2); /* pkg_id, user */
		VALIDATE_NOT_NULL(arg1);
		VALIDATE_NOT_NULL(arg2);

		g_dbus_method_invocation_return_value(inv, g_variant_new("(i)", package_run(p, arg1, arg2)));
	} else if (!g_strcmp0(method_name, "UMount")) {
		g_variant_get(parameters, "(i)", &argi);	/* Mount ref number */
		g_dbus_method_invocation_return_value(inv, g_variant_new("(i)", package_stop(p, (int) argi)));
	} else if (!g_strcmp0(method_name, "MountPointGet")) {
		char *mpoint;
		g_variant_get(parameters, "(s)", &arg1); /* pkg_id */
		VALIDATE_NOT_NULL(arg1);

		mpoint = package_mount_get(p, arg1);
		g_dbus_method_invocation_return_value(inv, g_variant_new("(is)", mpoint?1:DBP_ERROR_NOTFOUND, mpoint?mpoint:"!"));
		free(mpoint);
	} else if (!g_strcmp0(method_name, "RegisterPath")) {
		char *pkg_id, *mount, *dev;
		int ret;
		g_variant_get(parameters, "(s)", &arg1);	/* Path to package to register */
		VALIDATE_NOT_NULL(arg1);

		util_lookup_mount(arg1, &mount, &dev);
		ret = package_register_path(p, dev, arg1, mount, &pkg_id);
		g_dbus_method_invocation_return_value(inv, g_variant_new("(is)", ret, pkg_id?pkg_id:"!"));
		free(dev), free(mount), free(pkg_id);
	} else if (!g_strcmp0(method_name, "UnregisterPath")) {
		g_variant_get(parameters, "(s)", &arg1);	/* Path to unregister */
		VALIDATE_NOT_NULL(arg1);

		g_dbus_method_invocation_return_value(inv, g_variant_new("(i)", package_release_path(p, arg1)));
	} else if (!g_strcmp0(method_name, "IdFromPath")) {
		char *ret;
		g_variant_get(parameters, "(s)", &arg1);	/* Path to resolve ID from */
		VALIDATE_NOT_NULL(arg1);
		ret = package_id_from_path(p, arg1);
		g_dbus_method_invocation_return_value(inv, g_variant_new("(is)", ret?1:DBP_ERROR_NOTFOUND, ret?ret:"!"));
		free(ret);
	} else if (!g_strcmp0(method_name, "PathFromId")) {
		char *ret;
		g_variant_get(parameters, "(s)", &arg1);	/* PAckage Id to resolve path from */
		VALIDATE_NOT_NULL(arg1);
		ret = package_path_from_id(p, arg1);
		g_dbus_method_invocation_return_value(inv, g_variant_new("(is)", ret?1:DBP_ERROR_NOTFOUND, ret?ret:"!"));
		free(ret);
	} else if (!g_strcmp0(method_name, "Ping")) {
		g_dbus_method_invocation_return_value(inv, g_variant_new("(i)", 1));
	} else if (!g_strcmp0(method_name, "PackageList")) {
		int i;
		GVariantBuilder *builder;

		builder = g_variant_builder_new(G_VARIANT_TYPE("a(ssss)"));
		pthread_mutex_lock(&p->mutex);
		
		for (i = 0; i < p->entries; i++) {
			g_variant_builder_add(builder, "(ssss)", p->entry[i].path, p->entry[i].id, p->entry[i].desktop, p->entry[i].version);
		}

		if (!p->entries) {
			g_variant_builder_add(builder, "(ssss)", "/dev/null", "!", "nodesk", "");
		}
		
		pthread_mutex_unlock(&p->mutex);
		
		g_dbus_method_invocation_return_value(inv, g_variant_new("(a(ssss))", builder));
		g_variant_builder_unref(builder);
	} else if (!g_strcmp0(method_name, "Introspect")) {
		g_dbus_method_invocation_return_value(inv, g_variant_new("s", introspection_xml));
	} else {
		fprintf(dbp_error_log, "Bad method call %s\n", (const char *) method_name);
		return;
	}
	if (arg1)
		g_free(arg1);
	if (arg2)
		g_free(arg2);
}

void comm_dbus_announce(const char *name, const char *signame) {
	GError *local_error = NULL;

	if (!dconn) {
		sleep(2);
		if (!dconn)
			return;
	}
	
	g_dbus_connection_emit_signal(dconn, NULL, DBP_DBUS_DAEMON_OBJECT, DBP_DBUS_DAEMON_PREFIX, signame, g_variant_new("(s)", name), &local_error);
	g_assert_no_error(local_error);

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

static GVariant *vtab_get_prop(GDBusConnection *conn, const gchar *sender, const gchar *object_path, const gchar *interface_name, const char *property_name, GError **error, gpointer user_data) {
	(void) conn; (void) sender; (void) object_path; (void) interface_name; (void) property_name; (void) user_data;
	g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Properties unsupported\n");
	return NULL;
}

static gboolean vtab_set_prop(GDBusConnection *conn, const gchar *sender, const gchar *object_path, const gchar *interface_name, const char *property_name, GVariant *value, GError **error, gpointer user_data) {
	(void) conn; (void) sender; (void) object_path; (void) interface_name; (void) property_name; (void) value; (void) user_data;
	g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Properties unsupported\n");
	return 1;
}


static const GDBusInterfaceVTable iface_vtable = {
	vtab_method_call,
	vtab_get_prop,
	vtab_set_prop,
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


static void on_bus_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data) {
	(void) conn; (void) name; (void) user_data;
	if (g_dbus_connection_register_object(conn, DBP_DBUS_DAEMON_OBJECT, introspection_data->interfaces[0], &iface_vtable, NULL, NULL, NULL) <= 0) {
		fprintf(dbp_error_log, "Unable to register object %s on system bus\n", DBP_DBUS_DAEMON_OBJECT);
		daemon_exit_error = true;
		g_main_loop_quit(loop);
	}

	g_dbus_connection_register_object(conn, DBP_DBUS_DAEMON_OBJECT, introspection_data->interfaces[1], &iface_vtable, NULL, NULL, NULL);
	dconn = conn;
}


static void on_name_lost(GDBusConnection *conn, const gchar *name, gpointer user_data) {
	(void) name;
	(void) user_data;
	(void) conn;
	fprintf(dbp_error_log, "Unable to register bus %s on system bus\n", DBP_DBUS_DAEMON_PREFIX);
	daemon_exit_error = true;
	g_main_loop_quit(loop);
	return;
}


static void on_name_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data) {
	(void) conn;
	(void) name;
	(void) user_data;
	return;
}


void *comm_dbus_loop(void *n) {
	guint owner_id;
	FILE *fp;
	int sz;
	p = n;
	
	// Is apparently not needed on Debian Jessie and later
	//g_type_init();

	if (!(fp = fopen("/etc/dbp/dbpd-introspection.xml", "r"))) {
		daemon_exit_error = true;
		daemon_exited = true;
		pthread_exit(NULL);
	}

	fseek(fp, 0, SEEK_END), sz = ftell(fp), rewind(fp);
	introspection_xml = malloc(sz + 1);
	fread(introspection_xml, sz, 1, fp);
	fclose(fp);
	introspection_xml[sz] = 0;
	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

	owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, DBP_DBUS_DAEMON_PREFIX, G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
	loop = g_main_loop_new(NULL, false);
	g_main_loop_run(loop);
	g_bus_unown_name(owner_id);

	daemon_exited = true;
	fprintf(dbp_error_log, "dbus exit\n");

	pthread_exit(NULL);
	return NULL;
}

void comm_dbus_register(struct package_s *p) {
	pthread_t th;

	if (pthread_create(&th, NULL, comm_dbus_loop, p)) {
		fprintf(dbp_error_log, "Unable to create dbus listen thread\n");
		exit(1);
	}
}

void comm_kill() {
	g_main_loop_quit(loop);
}

int comm_died() {
	return daemon_exited;
}
