#ifndef __COMM_H__
#define	__COM__H__


void comm_dbus_init();
int comm_dbus_request_mountp(const char *pkg_id, char **reply);
int comm_dbus_request_mount(const char *pkg_id, const char *user);
void comm_dbus_request_umount(int run_id);


#endif
