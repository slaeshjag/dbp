#ifndef __COMM_H__
#define	__COMM_H__

void comm_dbus_register(struct package_s *p);
void comm_dbus_announce_new_meta(const char *id);
void comm_dbus_announce_rem_meta(const char *id);

#endif
