#ifndef __PACKAGE_H__
#define	__PACKAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include <archive.h>

struct package_entry_s {
	char				*path;
	char				*id;
	char				*device;
	char				*mount;
};


struct package_instance_s {
	char				*package_id;
	unsigned int			run_id;
	int				loop;
};


struct package_s {
	pthread_mutex_t			mutex;
	struct package_entry_s		*entry;
	int				entries;
	int				run_cnt;
	struct package_instance_s	*instance;
	int				instances;
};

struct package_s package_init();
void package_crawl_mount(struct package_s *p, const char *device, const char *path);
void package_release_mount(struct package_s *p, const char *device);
int package_run(struct package_s *p, const char *id, const char *user);
int package_stop(struct package_s *p, int run_id); 


#endif
