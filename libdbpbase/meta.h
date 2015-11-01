#ifndef __META_H__
#define	__META_H__

#ifdef _LIBINTERNAL
#include "desktop.h"
#endif


struct DBPMetaPackage {
	struct DBPDesktopFile		*df;
	const char			*section;
};

int dbp_meta_package_open(const char *path, struct DBPMetaPackage *mp);

#endif
