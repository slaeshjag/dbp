#ifndef __PACKAGE_H__
#define	__PACKAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include <archive.h>

struct package_entry_s {
	char			*path;
	char			*id;
	char			*device;
};


struct package_s {
	struct package_entry_s	*entry;
	int			entries;
};


#endif
