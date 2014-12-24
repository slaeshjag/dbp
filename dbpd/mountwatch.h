#ifndef __MOUNTWATCH_H__
#define	__MOUNTWATCH_H__

#include <pthread.h>
#include <semaphore.h>

#define	MOUNTWATCH_INOTIFY_MASK		(IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB)

enum mountwatch_tag_e {
	MOUNTWATCH_TAG_REMOVED,
	MOUNTWATCH_TAG_ADDED,
	MOUNTWATCH_TAG_CHANGED,
	MOUNTWATCH_TAG_PKG_ADDED,
	MOUNTWATCH_TAG_PKG_REMOVED,
};


struct mountwatch_entry_s {
	char				*mount;
	char				*device;
	char				*path;
	int				tag;
};


struct mountwatch_change_s {
	struct mountwatch_entry_s	*entry;
	int				entries;
};


struct mountwatch_inotify_s {
	char				*mount;
	char				*path;
	char				*device;
	int				handle;
};


struct mountwatch_s {
	sem_t				changed;
	struct mountwatch_entry_s	*entry;
	int				entries;

	struct mountwatch_inotify_s	*ientry;
	int				ientries;
	pthread_mutex_t			dir_watch_mutex;
	sem_t				dir_watch_continue;
	int				dir_change;
	int				dir_fd;

	int				should_die;
	int				died;
	int				died2;
};


extern struct mountwatch_s mountwatch_struct;


int mountwatch_init();
struct mountwatch_change_s mountwatch_diff();
void mountwatch_change_free(struct mountwatch_change_s change);
void mountwatch_kill();
int mountwatch_died();

#endif
