#ifndef __COMM_H__
#define	__COMM_H__

#include <stdint.h>

void comm_dbus_register_thumbnailer();
void comm_dbus_announce_started(uint32_t handle);
void comm_dbus_announce_finished(uint32_t handle);
void comm_dbus_announce_ready(uint32_t handle, const char *uri);
void comm_dbus_announce_error(uint32_t handle, const char *uri, enum ThumbError err, const char *msg);



#endif
