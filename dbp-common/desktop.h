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
struct desktop_file_s *desktop_parse_file(const char *path);
void *desktop_free(struct desktop_file_s *df);
void desktop_write(struct desktop_file_s *df, const char *path);
char *desktop_lookup(struct desktop_file_s *df, const char *key, const char *locale, const char *section);
int desktop_lookup_section(struct desktop_file_s *df, const char *section);
int desktop_lookup_entry(struct desktop_file_s *df, const char *key, const char *locale, int section);
int desktop_entry_new(struct desktop_file_s *df, const char *key, const char *locale, const char *value, int section);
int desktop_section_new(struct desktop_file_s *df, const char *name);

#endif
