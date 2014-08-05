#include "desktop.h"

/* A hybrid of a .desktop parser, and a .ini parser. Shouldn't cause too much
	trouble that it tries to do both */


static int desktop_section_new(struct desktop_file_s *df, const char *name) {
	int id;

	id = df->sections++;
	/* Lets just assume this will work... */
	df->section = realloc(df->section, sizeof(*df->section) * df->sections);

	df->section[id].name = name ? strdup(name) : NULL;
	df->section[id].entry = NULL, df->section[id].entries = 0;
	return id;
}


static int desktop_entry_new(struct desktop_file_s *df, const char *key, const char *locale, const char *value) {
	struct desktop_file_section_s *s;
	int e;

	s = &df->section[df->sections - 1];
	e = s->entries++;
	s->entry = realloc(s->entry, sizeof(*s->entry) * s->entries);
	s->entry[e].key = strdup(key), s->entry[e].locale = strdup(locale), s->entry[e].value = strdup(value);

	return e;
}


int desktop_lookup_section(struct desktop_file_s *df, const char *section) {
	int s, i;

	if (!section)
		s = 0;
	else
		for (i = 1; i < df->sections; i++)
			if (!strcmp(df->section[i].name, section)) {
				s = i;
				break;
			}
	return (i == df->sections) ? -1 : s;
}


int desktop_lookup_entry(struct desktop_file_s *df, const char *key, const char *locale, int section) {
	int i;

	if (section < 0) return -1;

	for (i = 0; i < df->section[section].entries; i++)
		if (!strcmp(df->section[section].entry[i].key, key))
			if (!strcmp(df->section[section].entry[i].locale, locale))
				return i;
	return -1;
}


char *desktop_lookup(struct desktop_file_s *df, const char *key, const char *locale, const char *section) {
	char *locbuff;
	int s, i;

	locbuff = strdup(locale);
	/* Strip out variant */
	if (strchr(locbuff, '@'))
		*strchr(locbuff, '@') = 0;
	if ((s = desktop_lookup_section(df, section)) < 0) {
		free(locbuff);
		return NULL;
	}

	if ((i = desktop_lookup_entry(df, key, locale, s)) >= 0);
	else if ((i = desktop_lookup_entry(df, key, locbuff, s)) >= 0);
	else if ((i = desktop_lookup_entry(df, key, "", s)) >= 0);
	else {
		free(locbuff);
		return NULL;
	}

	free(locbuff);
	return df->section[s].entry[i].value;
}


struct desktop_file_s *desktop_parse(char *str) {
	/* TODO: Make these dynamically reallocable */
	char key[4096], value[4096], buff[4096], buff2[4096], *tmp;

	int sz, brk;
	struct desktop_file_s *df;

	df = malloc(sizeof(*df));
	df->sections = 0, df->section = NULL;
	desktop_section_new(df, NULL);

	/* Hack to make the parser work with retarded line endings */
	while (strchr(str, '\r'))
		*strchr(str, '\r') = '\n';

	for (brk = 0; !brk; str = strchr(str, '\n') + 1) {
		if (!(tmp = strchr(str, '\n')))
			sz = strlen(str), brk = 1;
		else
			sz = tmp - str;

		/* TODO: Handle too long strings */
		if (sz >= 4096)
			continue;
	
		*value = 0, *key = 0;
		sscanf(str, "%[^\n=] = %[^\n]\n", key, value);
		if (!(*key))
			continue;
		if (*key == '[') {	/* This is a new section */
			if (!strchr(key, ']'))
				continue;
			*strchr(key, ']') = 0;
			sscanf(key + 1, "%[^\n]", buff);
			desktop_section_new(df, buff);
		} else if (*key == ';' || *key == '#') {
		} else {
			if (strchr(key, '[')) {	/* locale string */
				if (!strchr(key, ']'))
					continue;	/* nope.avi */
				*strchr(key, '[') = '\n', *strchr(key, ']') = 0;
				sscanf(key, "%[^\n]\n%[^\n]", buff, buff2);
				desktop_entry_new(df, buff, buff2, value);
			} else
				desktop_entry_new(df, key, "", value);
		}
	}
	
	return df;
}


struct desktop_file_s *desktop_parse_file(const char *path) {
	FILE *fp;
	char *buff;
	long file_sz;
	struct desktop_file_s *df;

	if (!(fp = fopen(path, "r")))
		return NULL;
	fseek(fp, 0, SEEK_END);
	file_sz = ftell(fp);
	rewind(fp);
	buff = malloc(file_sz + 1);
	buff[fread(buff, 1, file_sz, fp)] = 0;
	fclose(fp);
	df = desktop_parse(buff);
	free(buff);
	return df;
}


void desktop_write(struct desktop_file_s *df, const char *path) {
	FILE *fp;
	int i, j;

	if (!(fp = fopen(path, "w")))
		return;
	for (i = 0; i < df->sections; i++) {
		if (df->section[i].name)
			fprintf(fp, "[%s]\n", df->section[i].name);
		for (j = 0; j < df->section[i].entries; j++)
			if (!*df->section[i].entry[j].key)
				continue;
			else if (*df->section[i].entry[j].locale)
				fprintf(fp, "%s[%s]=%s\n", df->section[i].entry[j].key, df->section[i].entry[j].locale, df->section[i].entry[j].value);
			else
				fprintf(fp, "%s=%s\n", df->section[i].entry[j].key, df->section[i].entry[j].value);
	}

	fclose(fp);

	return;
}
				

void *desktop_free(struct desktop_file_s *df) {
	int i, j;

	if (!df) return NULL;
	for (i = 0; i < df->sections; i++) {
		free(df->section[i].name);
		for (j = 0; j < df->section[i].entries; j++) {
			free(df->section[i].entry[j].key);
			free(df->section[i].entry[j].locale);
			free(df->section[i].entry[j].value);
		}

		free(df->section[i].entry);
	}

	free(df->section);
	free(df);

	return NULL;
}
