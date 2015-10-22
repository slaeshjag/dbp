#ifndef __DBP_TYPES_H__
#define	__DBP_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

struct DBPList {
	struct DBPList			*next;
	char				*id;
	char				*path;
	bool				on_desktop;
};


struct DBPDepend {
	char				*pkg_name;
	struct {
		char			*lt;
		char			*lteq;
		char			*eq;
		char			*gt;
		char			*gteq;
	} version;
	char				*arch;
};


struct DBPDependDPackage {
	char				*name;
	char				*version;
	char				*arch;
	struct DBPDependDPackage	*next;

};

struct DBPDependDPackageNode {
	struct DBPDependDPackage	*match;
	struct DBPDependDPackageNode	**lookup;
	struct DBPDependDPackage	*list;
	int				list_size;
};

#endif
