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

void package_crawl(struct package_s *p, const char *device, const char *path);
void package_crawl_mount(struct package_s *p, const char *device, const char *path);


#endif
