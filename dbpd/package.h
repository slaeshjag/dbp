#ifndef __PACKAGE_H__
#define	__PACKAGE_H__

#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <pthread.h>
#include <archive.h>


struct package_entry_s {
	char				*path;
	char				*id;
	char				*desktop;

	char				*device;
	char				*mount;
	char				*appdata;

	/* Daemon does not check version, but includes it in package list */
	char				*version;

	/* Dependencies are only stored for later relay to client	*
	** Daemon does not do any dep-checking itself			*/
	char				*sys_dep;
	char				*pkg_dep;

	char				**exec;
	int				execs;
};


struct package_instance_s {
	char				*package_id;
	unsigned int			run_id;
	int				loop;
};

struct package_purgatory_s {
	char				*package_id;
	int				loop_number;
	int				reusable;
	time_t				expiry;
};

struct package_s {
	pthread_mutex_t			mutex;
	struct package_entry_s		*entry;
	int				entries;
	int				run_cnt;
	struct package_instance_s	*instance;
	int				instances;
	struct package_purgatory_s	*purgatory;
	int				purgatory_entries;

};

struct package_s package_init();
void package_crawl_mount(struct package_s *p, const char *device, const char *path);
void package_release_mount(struct package_s *p, const char *device);
int package_register_path(struct package_s *p, const char *device, const char *path, const char *mount, char **pkg_id);
int package_release_path(struct package_s *p, const char *path);
int package_run(struct package_s *p, const char *id, const char *user);
int package_stop(struct package_s *p, int run_id); 
char *package_mount_get(struct package_s *p, const char *pkg_id);
char *package_id_from_path(struct package_s *p, const char *path);
char *package_appdata_from_id(struct package_s *p, const char *id);
int package_deps_from_id(struct package_s *p, const char *id, char **sys, char **pkg);
char *package_path_from_id(struct package_s *p, const char *id);
void package_purgatory_check(struct package_s *p);


#endif
