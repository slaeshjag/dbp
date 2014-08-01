#ifndef __COMM_H__
#define	__COM__H__


void comm_dbus_init();
int comm_dbus_request_mountp(const char *pkg_id, char **reply);
int comm_dbus_request_mount(const char *pkg_id, const char *user);
void comm_dbus_request_umount(int run_id);
int comm_dbus_register_path(const char *path, char **id);
int comm_dbus_unregister_path(const char *path);
int comm_dbus_get_id_from_path(const char *path, char **rets);
int comm_dbus_get_appdata(const char *id, char **rets);


#endif
