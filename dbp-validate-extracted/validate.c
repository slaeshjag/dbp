#define	_GNU_SOURCE
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <dbpbase/dbpbase.h>
#include <dbpbase/desktop.h>

#include "validate.h"
#include "categories.h"

#define	_(STRING)		gettext(STRING)
#define	LOG_FREE(msg, dest)	free(add_string_message(&(dest), (msg)))
#define	LOG(msg, dest)		add_string_message(&(dest), (msg))


struct StringBuffer {
	char *str;
	struct StringBuffer	*next;
};

struct StringBufferContainer {
	struct StringBuffer	*first;
	struct StringBuffer	*last;
	int			count;
};


struct CheckList {
	char			*name;
	char			*full_name;
	bool			check;
	struct CheckList	*next;
};


struct StringBufferContainer notice, warning, error;
struct CheckList *icons_avail, *icons_used, *launchers_to_skip, *execs_used, *desktop_files_to_scan;
char *rootfs_path = NULL, *meta_path = NULL;

bool directorylist_to_checklist(char *name, struct CheckList **list, char *prefix, char *suffix);

void checklist_add(char *str, char *name, struct CheckList **list) {
	struct CheckList *new;

	new = malloc(sizeof(*new));
	new->check = false;
	new->name = strdup(name);
	new->full_name = strdup(str);
	new->next = *list;
	*list = new;
	return;
}


struct CheckList *checklist_find(char *str, struct CheckList *list) {
	if (!list)
		return NULL;
	if (!strcmp(list->name, str))
		return list;
	return checklist_find(str, list->next);
}


char *add_string_message(struct StringBufferContainer *cont, char *string) {
	struct StringBuffer *new = malloc(sizeof(*new));
	new->next = NULL;
	if (cont->last) {
		cont->last->next = new;
		cont->last = new;
	} else {
		cont->first = new;
		cont->last = new;
	}

	cont->count++;
	new->str = strdup(string);
	return string;
}


void usage() {
	fprintf(stdout, _("Validates ingoing files before a DBP is created\n"));
	fprintf(stdout, _("By Steven Arnow, 2015-2016, version %s\n"), dbp_config_version_get());
	fprintf(stdout, "\n");
	fprintf(stdout, _("Usage:\n"));
	fprintf(stdout, _("dbp-validate-extracted <path to meta directory> [path to rootfs directory]\n"));
}


void print_messages() {
	struct StringBuffer *next;
	for (next = error.first; next; next = next->next)
		fprintf(stdout, "ERROR: %s\n", next->str);
	for (next = warning.first; next; next = next->next)
		fprintf(stdout, "WARNING: %s\n", next->str);
	for (next = notice.first; next; next = next->next)
		fprintf(stdout, "NOTICE: %s\n", next->str);
	if (error.count || warning.count || notice.count)
		fprintf(stdout, "%i errors, %i warnings, %i notices\n", error.count, warning.count, notice.count);
}


static struct DBPDefaultMetaKey *__lookup_metakey(char *meta) {
	int i;

	for (i = 0; default_mk[i].name; i++)
		if (!strcmp(meta, default_mk[i].name))
			return &default_mk[i];
	return NULL;
}


bool _conforms_strict_name_type(char *str) {
	if (*str == '.' || *str == 0)
		return false;
	for (; *str; str++)
		if (!isascii(*str) || !(isalnum(*str) || *str == '.' || *str == '_' || *str == '-'))
			return false;
	return true;
}


bool _contains_uppercase(const char *str) {
	for (; *str; str++)
		if (isupper(*str))
			return true;
	return false;
}


struct CategoryEntry *_find_category(const char *cat) {
	int i;

	for (i = 0; freedesktop_cat[i].name; i++)
		if (!strcmp(freedesktop_cat[i].name, cat))
			return &freedesktop_cat[i];
	return NULL;
}


bool _valid_parent_category(struct CategoryEntry *ce, const char *prev) {
	int i;
	
	if (!ce->requires_parent)
		return true;
	if (!prev)
		return false;
	
	for (i = 0; ce->parents[i]; i++)
		if (!strcmp(ce->parents[i], prev))
			return true;
	return false;
}


bool _valid_category(const char *prev, const char *cat) {
	int i, j;

	for (i = 0; freedesktop_cat[i].name; i++) {
		if (strcmp(freedesktop_cat[i].name, cat))
			continue;
		if (!freedesktop_cat[i].requires_parent)
			return true;
		if (!prev)
			return false;
		for (j = 0; freedesktop_cat[i].parents[j]; j++)
			if (!strcmp(freedesktop_cat[i].parents[j], prev))
				return true;
		return false;
	}

	return false;
}


void _find_unused_icons() {
	struct CheckList *next;
	char *tmp;

	for (next = icons_avail; next; next = next->next)
		if (!checklist_find(next->name, icons_used))
			LOG_FREE((asprintf(&tmp, "Icon file %s is present but not used", next->name), tmp), warning);
}


bool validate_desktop_file(const char *path) {
	int section;
	struct DBPDesktopFile *df;
	char *tmp;
	bool valid = true;

	if (!(df = dbp_desktop_parse_file(path)))
		return LOG_FREE((asprintf(&tmp, "Desktop file %s could not be opened", path), tmp), warning), false;
	if ((section = dbp_desktop_lookup_section(df, "Desktop Entry")) < 0)
		return LOG_FREE((asprintf(&tmp, "Desktop file %s does not have a desktop entry", path), tmp), warning), false;
	if (!(tmp = dbp_desktop_lookup(df, "Type", NULL, "Desktop Entry")))
		return LOG_FREE((asprintf(&tmp, "Desktop file %s does not specify a type in its desktop entry", path), tmp), error), false;
	
	if (!strcmp(tmp, "Application")) {
		if (!dbp_desktop_lookup(df, "Exec", NULL, "Desktop Entry") && !dbp_desktop_lookup(df, "TryExec", NULL, "Desktop Entry"))
			LOG_FREE((asprintf(&tmp, "Desktop file %s does not provide a Exec or TryExec entry, which is mandatory for applications", path), tmp), error), valid = false;
	} else if (!strcmp(tmp, "Link")) {
		if (!(tmp = dbp_desktop_lookup(df, "URL", NULL, "Desktop Entry")) || !strlen(tmp))
			LOG_FREE((asprintf(&tmp, "Desktop file %s does not provide a URL entry, which is mandatory for links", path), tmp), error), valid = false;
	} else if (!strcmp(tmp, "Directory")) {
		
	} else
		LOG_FREE((asprintf(&tmp, "Desktop file %s have an invalid type '%s' in its desktop entry", path, tmp), tmp), error), valid = false;
	
	/* TODO: Check that all present keys are known */

	if (!(tmp = dbp_desktop_lookup(df, "Name", NULL, "Desktop Entry")) || !strlen(tmp))
		LOG_FREE((asprintf(&tmp, "Desktop file %s does not have a name set in its desktop entry", path), tmp), error), valid = false;
	if (!(tmp = dbp_desktop_lookup(df, "Comment", NULL, "Desktop Entry")) || !strlen(tmp))
		LOG_FREE((asprintf(&tmp, "Desktop file %s is missing a comment in its desktop entry", path), tmp), warning);
	if (!(tmp = dbp_desktop_lookup(df, "Icon", NULL, "Desktop Entry")) || !strlen(tmp))
		LOG_FREE((asprintf(&tmp, "Desktop file %s is missing an icon in its desktop entry", path), tmp), warning);
	else {
		checklist_add(tmp, tmp, &icons_used);
		if (!checklist_find(tmp, icons_avail))
			LOG_FREE((asprintf(&tmp, "Desktop file %s uses the icon %s, which is missing", path, tmp), tmp), error), valid = false;
	}
	
	/* Check categories */ {
		char *category = NULL, **catlist = NULL, *last;
		int categories = 0, i;
		if (!(category = dbp_desktop_lookup(df, "Categories", NULL, "Desktop Entry")))
			return LOG_FREE((asprintf(&tmp, "Desktop file %s does not have any categories set in its desktop entry", path), tmp), error), false;
		category = strdup(category);
		if (category[strlen(category) - 1] != ';')
			LOG_FREE((asprintf(&tmp, "Desktop file %s has categories, but the list isn't terminated with a ';'", path), tmp), error);
		else
			category[strlen(category) - 1] = 0;
			
		dbp_config_expand_token(&catlist, &categories, category);
		
		for (i = 0, last = NULL; i < categories; i++) {
			struct CategoryEntry *ce;

			if (!(ce = _find_category(catlist[i])))
				LOG_FREE((asprintf(&tmp, "Desktop file %s uses invalid category '%s'", path, catlist[i]), tmp), error), valid = false;
			else if (!_valid_parent_category(ce, last)) {
				if (!ce->parents[0])
					LOG_FREE((asprintf(&tmp, "Desktop file %s uses the subcategory '%s', but a main category must preceed it", path, catlist[i]), tmp), error), valid = false;
				else if (!ce->parents[1])
					LOG_FREE((asprintf(&tmp, "Desktop file %s uses the subcategory '%s', but the category '%s' must preceed it", path, catlist[i], ce->parents[0]), tmp), error), valid = false;
				else if (!ce->parents[2])
					LOG_FREE((asprintf(&tmp, "Desktop file %s uses the subcategory '%s', but the category '%s' or '%s' must preceed it", path, catlist[i], ce->parents[0], ce->parents[1]), tmp), error), valid = false;
				else
					LOG_FREE((asprintf(&tmp, "Desktop file %s uses the subcategory '%s', but the category '%s', '%s' or '%s' must preceed it", path, catlist[i], ce->parents[0], ce->parents[1], ce->parents[2]), tmp), error), valid = false;
			}

			last = catlist[i];
		}
	}

	return valid;
}


bool validate_package_data(struct DBPDesktopFile *def) {
	int section;
	char *path, *tmp;

	asprintf(&path, "%s/icons", meta_path);
	if (!directorylist_to_checklist(path, &icons_avail, NULL, ".png"))
		LOG_FREE((asprintf(&tmp, "Icon directory '%s' is missing", path), tmp), warning);
	if ((section = dbp_desktop_lookup_section(def, "Package Entry")) < 0)
		return LOG("default.desktop is missing the [Package Entry] section", error), false;
	if (!(tmp = dbp_desktop_lookup(def, "Id", NULL, "Package Entry")))
		LOG("default.desktop is missing its 'Id' entry", error);
	else if (!_conforms_strict_name_type(tmp))
		LOG("default.desktop has a package ID specified, but it's invalid", error);
	else if (_contains_uppercase(tmp))
		LOG("default.desktop has a package ID containg upper-case characters", error);
	if (!(tmp = dbp_desktop_lookup(def, "Icon", NULL, "Package Entry")))
		LOG("default.desktop does not have an icon specified in its Package Entry", warning);
	else {
		checklist_add(tmp, tmp, &icons_used);
		if (!checklist_find(tmp, icons_avail))
			LOG_FREE((asprintf(&path, "default.desktop has the icon '%s' specified in its Package Entry, which is missing", tmp), path), error);
	}
		

	if (!(tmp = dbp_desktop_lookup(def, "Appdata", NULL, "Package Entry")));
	else if (!_conforms_strict_name_type(tmp))
		LOG("default.desktop has an appdata directory specified, but it contains illegal characters", error);
	
	/* Check package ID etc. etc. */
	if (dbp_desktop_lookup_section(def, "Desktop Entry") < 0)
		LOG("default.desktop is lacking a [Desktop Entry], this package will not have a default launch action", warning);
	free(path);
	asprintf(&path, "%s/meta", meta_path);
	directorylist_to_checklist(path, &desktop_files_to_scan, NULL, ".desktop");
	if (!icons_avail)
		LOG("No icons found, this is probably not what you want", warning);
	if (!desktop_files_to_scan)
		LOG("No desktop files found", warning);
	
	/* TODO: Check that exported execs are present */
	/* TODO: Check for illegal characters in dependencies */

	return true;
}


bool directorylist_to_checklist(char *path, struct CheckList **list, char *prefix, char *suffix) {
	DIR *d;
	struct dirent dir, *result;
	char *tmpnam;

	if (!(d = opendir(path)))
		return false;
	for (readdir_r(d, &dir, &result); result; readdir_r(d, &dir, &result)) {
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))	// don't add . and ..
			continue;
		if (prefix && strncmp(prefix, path, strlen(prefix)))
			continue;
		if (suffix && strlen(suffix) > strlen(dir.d_name))
			continue;
		if (suffix && strcmp(suffix, dir.d_name + strlen(dir.d_name) - strlen(suffix)))
			continue;
		asprintf(&tmpnam, "%s/%s", path, dir.d_name);
		checklist_add(tmpnam, dir.d_name, list);
		free(tmpnam);
	}

	closedir(d);
	return true;
}


int main(int argc, char **argv) {
	struct CheckList *next;
	struct DBPDesktopFile *default_desk;

	if (argc < 2)
		return usage(), 1;
	if (argc <3)
		LOG("No rootfs given, assuming meta-only package", notice);
	else
		rootfs_path = argv[2];
	meta_path = argv[1];

	{
		char *tmp, *tmp2;
		asprintf(&tmp, "%s/meta/default.desktop", meta_path);
		if (!(default_desk = dbp_desktop_parse_file(tmp))) {
			LOG_FREE((asprintf(&tmp2, "Missing default.desktop [%s]", tmp), tmp2), error);
			free(tmp);
			goto fatal;
		}
		free(tmp);
	}

	validate_package_data(default_desk);

	for (next = desktop_files_to_scan; next; next = next->next)
		validate_desktop_file(next->full_name);
	_find_unused_icons();

	fatal:
	print_messages();

	return !!error.count;
}
