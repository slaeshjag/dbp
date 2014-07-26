#ifndef __DBP_H__
#define	__DBP_H__


#define	DBP_DBUS_CLIENT_PREFIX	"de.dragonbox.DBP.run"
#define	DBP_DBUS_DAEMON_PREFIX	"de.dragonbox.PackageDaemon"
#define	DBP_DBUS_DAEMON_OBJECT	"/de/dragonbox/PackageDaemon"


/* ERROR CODES */

#define	DBP_ERROR_NO_REPLY	-1000	/* dbp daemon did not reply */
#define	DBP_ERROR_INTERNAL_MSG	-1001	/* internal dbp message error */
#define	DBP_ERROR_NO_LOOP	-1002	/* No loop device available */
#define	DBP_ERROR_SET_LOOP	-1003	/* Error setting up loop device */
#define	DBP_ERROR_SET_LOOP2	-1004	/* Error setting up loop device */
#define	DBP_ERROR_NO_PKG_ACCESS	-1005	/* Package file couldn't be opened */

#endif
