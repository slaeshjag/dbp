#ifndef __DESKTOP_H__
#define	__DESKTOP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct desktop_file_entry_s {
	char				*key;
	char				*locale;
	char				*value;
};


struct desktop_file_section_s {
	char				*name;
	struct desktop_file_entry_s	*entry;
	int				entries;
};


struct desktop_file_s {
	struct desktop_file_section_s	*section;
	int				sections;
};


struct desktop_file_s *desktop_parse(char *str);
void *desktop_free(struct desktop_file_s *df);
void desktop_dump(struct desktop_file_s *df);


#endif
