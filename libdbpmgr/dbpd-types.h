#ifndef __DBP_TYPES_H__
#define	__DBP_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

struct DBPList {
	struct DBPList		*next;
	char			*id;
	char			*path;
	bool			on_desktop;
};


#endif
