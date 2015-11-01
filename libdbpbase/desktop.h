#ifndef __DBP_DESKTOP_H__
#define	__DBP_DESKTOP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct DBPDesktopFileEntry {
	char				*key;
	char				*locale;
	char				*value;
};


struct DBPDesktopFileSection {
	char				*name;
	struct DBPDesktopFileEntry	*entry;
	int				entries;
};


struct DBPDesktopFile {
	struct DBPDesktopFileSection	*section;
	int				sections;
};


struct DBPDesktopFile *dbp_desktop_parse(char *str);
struct DBPDesktopFile *dbp_desktop_parse_append(char *str, struct DBPDesktopFile *df);
struct DBPDesktopFile *dbp_desktop_parse_file(const char *path);
struct DBPDesktopFile *dbp_desktop_parse_file_append(const char *path, struct DBPDesktopFile *df);
void *dbp_desktop_free(struct DBPDesktopFile *df);
void dbp_desktop_write(struct DBPDesktopFile *df, const char *path);
char *dbp_desktop_lookup(struct DBPDesktopFile *df, const char *key, const char *locale, const char *section);
int dbp_desktop_lookup_section(struct DBPDesktopFile *df, const char *section);
int dbp_desktop_lookup_entry(struct DBPDesktopFile *df, const char *key, const char *locale, int section);
int dbp_desktop_entry_new(struct DBPDesktopFile *df, const char *key, const char *locale, const char *value, int section);
int dbp_desktop_section_new(struct DBPDesktopFile *df, const char *name);

#endif
