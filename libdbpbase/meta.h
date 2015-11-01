#ifndef __META_H__
#define	__META_H__

#include "desktop.h"


struct meta_package_s {
	struct desktop_file_s		*df;
	const char			*section;
};

int meta_package_open(const char *path, struct meta_package_s *mp);

#endif
