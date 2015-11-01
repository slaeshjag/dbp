#ifndef __DBP_H__
#define	__DBP_H__

#include <stdio.h>

#define	DBP_META_PREFIX		"__dbp__"

#define	DBP_GETTEXT_DOMAIN	"dbp-run"
#define	DBP_DBUS_CLIENT_PREFIX	"de.dragonbox.DBP.run"
#define	DBP_DBUS_DAEMON_PREFIX	"de.dragonbox.PackageDaemon"
#define	DBP_DBUS_DAEMON_OBJECT	"/de/dragonbox/PackageDaemon"
#define	DBP_DBUS_THUMB_PREFIX	"de.dragonbox.PackageThumbnail"
#define	DBP_DBUS_THUMB_OBJECT	"/de/dragonbox/PackageThumbnail"

#define	DBP_FS_NAME		"squashfs"
#define	DBP_UNIONFS_NAME	"aufs"



/* ERROR CODES */

#define	DBP_ERROR_NO_REPLY	-1000	/* dbp daemon did not reply */
#define	DBP_ERROR_INTERNAL_MSG	-1001	/* internal dbp message error */
#define	DBP_ERROR_NO_LOOP	-1002	/* No loop device available */
#define	DBP_ERROR_SET_LOOP	-1003	/* Error setting up loop device */
#define	DBP_ERROR_SET_LOOP2	-1004	/* Error setting up loop device */
#define	DBP_ERROR_NO_PKG_ACCESS	-1005	/* Package file couldn't be opened */
#define	DBP_ERROR_NO_MEMORY	-1006	/* A malloc failed or something */
#define	DBP_ERROR_BAD_PKG_ID	-1007	/* Package doesn't exist in database */
#define	DBP_ERROR_BAD_FSIMG	-1008	/* Package doesn't have a valid FS */
#define	DBP_ERROR_ILL_DIRNAME	-1009	/* A mountpoint contained an illegal char */
#define	DBP_ERROR_UNION_FAILED	-1010	/* UnionFS mount failed */
#define	DBP_ERROR_APPD_NOPERM	-1011	/* Unable to create appdata directory */
#define	DBP_ERROR_NO_DEFAULTD	-1012	/* No default.desktop in the package */
#define	DBP_ERROR_PKG_REG	-1013	/* Package is already registered */
#define	DBP_ERROR_UNHANDLED	-1014	/* An unhandled error occured */
#define	DBP_ERROR_BAD_META	-1015	/* Unable to access package metadata */
#define	DBP_ERROR_NO_DEFAULTH	-1016	/* No default handler in package */
#define	DBP_ERROR_SIGEXIT	-1017	/* Program problably died from a signal */
#define	DBP_ERROR_SIGSEGV	-1018	/* Program died from segfault */
#define	DBP_ERROR_MYSTKILL	-1019	/* Program died from unhandled reason */
#define	DBP_ERROR_NOTFOUND	-1020	/* Generic parameter-not-found */
#define	DBP_ERROR_NOTVALID	-1021	/* Generic invalid parameter (sanity check failed) */
#define	DBP_ERROR_BADEXT	-1022	/* This file is has an uninteresting extension */
#define	DBP_ERROR_MUTILATED	-1023	/* Uncleanly removed media with active programs */

extern FILE *dbp_error_log;
int dbp_init(FILE *dbp_error_log);

#endif
