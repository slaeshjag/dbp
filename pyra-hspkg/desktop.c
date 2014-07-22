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


struct desktop_file_s *desktop_parse(char *str) {
	/* TODO: Make these dynamically reallocable */
	char key[4096], value[4096], buff[4096], buff2[4096], *tmp;

	int sz, brk, csec;
	struct desktop_file_s *df;

	df = malloc(sizeof(*df));
	df->sections = 0, df->section = NULL;
	csec = desktop_section_new(df, NULL);

	for (brk = 0; !brk; str = strchr(str, '\n') + 1) {
		if (!(tmp = strchr(str, '\n')))
			sz = strlen(str), brk = 1;
		else
			sz = tmp - str;
		/* TODO: Use sz to determine if key, value is big enough */
	
		*value = 0, *key = 0;
		sscanf(str, "%[^\n=] = %[^\n]\n", key, value);
		if (*key == '[') {	/* This is a new section */
			if (!strchr(key, ']'))
				continue;
			*strchr(key, ']') = 0;
			sscanf(key + 1, "%[^\n]", buff);
			csec = desktop_section_new(df, buff);
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


void *desktop_free(struct desktop_file_s *df) {
	int i, j;

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

void desktop_dump(struct desktop_file_s *df) {
	int i, j;

	for (i = 0; i < df->sections; i++) {
		fprintf(stdout, "SECTION %s\n", df->section[i].name);
		for (j = 0; j < df->section[i].entries; j++)
			fprintf(stdout, "%s [%s] = %s\n", df->section[i].entry[j].key, df->section[i].entry[j].locale, df->section[i].entry[j].value);
	}

	return;
}
