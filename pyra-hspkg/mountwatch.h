#ifndef __MOUNTWATCH_H__
#define	__MOUNTWATCH_H__

#include <pthread.h>
#include <semaphore.h>

enum mountwatch_tag_e {
	MOUNTWATCH_TAG_REMOVED,
	MOUNTWATCH_TAG_ADDED,
	MOUNTWATCH_TAG_CHANGED,
};


struct mountwatch_entry_s {
	char				*mount;
	char				*device;
	int				tag;
};


struct mountwatch_change_s {
	struct mountwatch_entry_s	*entry;
	int				entries;
};


struct mountwatch_s {
	sem_t				changed;
	struct mountwatch_entry_s	*entry;
	int				entries;
};


extern struct mountwatch_s mountwatch_struct;


int mountwatch_init();
struct mountwatch_change_s mountwatch_diff();
void mountwatch_change_free(struct mountwatch_change_s change);

#endif
