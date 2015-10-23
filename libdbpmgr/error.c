/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'error.c' - This file is part of libdbpmgr

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/

#include <libintl.h>
#include "dbp.h"

#define	_(STRING)	dgettext(DBP_GETTEXT_DOMAIN, STRING)


const char *dbpmgr_error_lookup(int error) {
	fprintf(stderr, "Decoding %i\n", error);
	if (error == DBP_ERROR_NO_REPLY)
		return _("The DBP System Daemon is not responding. Make sure it's running");
	if (error == DBP_ERROR_INTERNAL_MSG)
		return _("An unknown internal error has occured. This is only supposed to be a placeholder. You should examine the DBP System daemon log-file for further information.");
	if (error == DBP_ERROR_NO_LOOP)
		return _("The system has ran out of loopback device nodes. Try closing some applications and try again.");
	if (error == DBP_ERROR_SET_LOOP)
		return _("The system has ran out of loopback device nodes. Try closing some applications and try again.");
	if (error == DBP_ERROR_SET_LOOP2)
		return _("The DBP System Daemon was unable to assign the package file to a device node. ioctl failed.");
	if (error == DBP_ERROR_NO_PKG_ACCESS)
		return _("The package file could not be opened. Make sure it's still accessable.");
	if (error == DBP_ERROR_NO_MEMORY)
		return _("Unable to allocate system resources. You're probably out of RAM. Close some programs and try again.");
	if (error == DBP_ERROR_BAD_PKG_ID)
		return _("You've attempted to launch a program from a package that could not be found in the local database. This is likely a bug.");
	if (error == DBP_ERROR_BAD_FSIMG)
		return _("Unable to access the package filesystem. This DBP-file is likely corrupt.");
	if (error == DBP_ERROR_ILL_DIRNAME)
		return _("The meta-data in this package is malformed: Illegal character(s) were found in a generated mountpoint.");
	if (error == DBP_ERROR_UNION_FAILED)
		return _("Unable to set up AuFS union. This is likely a bug. This may temporarily be worked around by rebooting.");
	if (error == DBP_ERROR_APPD_NOPERM)
		return _("Your user doesn't have the permissions needed to create the application data directory on the SD-card that this application is installed on.");
	if (error == DBP_ERROR_NO_DEFAULTD)
		return _("No default.desktop was found in the meta-data for this package.");
	if (error == DBP_ERROR_PKG_REG)
		return _("The package with this ID is already registered! This is not an error, and you shouldn't see this message.");
	if (error == DBP_ERROR_UNHANDLED)
		return _("An unhandled error has occured. This is just a placehoder that you shouldn't see.");
	if (error == DBP_ERROR_BAD_META)
		return _("Unable to read meta-data from package. The package file is likely corrupt. Try re-downloading it.");
	if (error == DBP_ERROR_NO_DEFAULTH)
		return _("This package lacks a default executable, and therefore cannot be executed directly.");
	if (error == DBP_ERROR_SIGEXIT)
		return _("The program was killed by a signal");
	if (error == DBP_ERROR_SIGSEGV)
		return _("The program has crashed from a segmentation fault. This likely a bug in the program.");
	if (error == DBP_ERROR_MYSTKILL)
		return _("The program exited abnormally for an unknown reason.");
	if (error == DBP_ERROR_NOTFOUND)
		return _("Could not find one or more resource specified in the given parameter(s).");
	if (error == DBP_ERROR_NOTVALID)
		return _("One or more parameters contains malformed data that can not be processed.");
	if (error == DBP_ERROR_BADEXT)
		return _("The given file does not have a recognized file extension.");
	if (error == DBP_ERROR_MUTILATED)
		return _("Instances of this program are still running after an unclean media removal, and it is not safe to spawn new instances. Close the old instances of this program before trying again.");
	return _("An unknown error occured.");
}
