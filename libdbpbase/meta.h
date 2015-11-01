#ifndef __META_H__
#define	__META_H__

#include "desktop.h"


struct DBPMetaPackage {
	struct DBPDesktopFile		*df;
	const char			*section;
};

int dbp_meta_package_open(const char *path, struct DBPMetaPackage *mp);

#endif
