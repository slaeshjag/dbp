#include "desktop.h"
#include "dbp.h"

/* A hybrid of a .desktop parser, and a .ini parser. Shouldn't cause too much
	trouble that it tries to do both */


int desktop_section_new(struct desktop_file_s *df, const char *name) {
	int id;

	for (id = 0; id < df->sections; id++)
		if (!strcmp(df->section[id].name, name))
			return id;

	id = df->sections++;
	/* Lets just assume this will work... */
	df->section = realloc(df->section, sizeof(*df->section) * df->sections);

	df->section[id].name = name ? strdup(name) : NULL;
	df->section[id].entry = NULL, df->section[id].entries = 0;
	return id;
}


int desktop_entry_new(struct desktop_file_s *df, const char *key, const char *locale, const char *value, int section) {
	struct desktop_file_section_s *s;
	int e, i;

	for (i = 0; i < df->section[section].entries; i++)
		if (!strcmp(df->section[section].entry[i].key, key) && !strcmp(df->section[section].entry[i].locale, locale))
			return i;	// Key/value collision, keep the old one
	s = &df->section[df->sections - 1];
	e = s->entries++;
	s->entry = realloc(s->entry, sizeof(*s->entry) * s->entries);
	s->entry[e].key = strdup(key), s->entry[e].locale = strdup(locale), s->entry[e].value = strdup(value);

	return e;
}


int desktop_lookup_section(struct desktop_file_s *df, const char *section) {
	int s = -1, i = 0;
	
	if (!df) return -1;

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

	if (!df) return -1;
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
	
	if (!df) return NULL;

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


static void desktop_entry_unescape(char *str) {
	int i, len;
	len = strlen(str) + 1;
	for (i = 0; i < len; i++) {
		if (str[i] != '\\')
			continue;
		i++;
		if (!str[i])
			continue;
		if (str[i] == '\\')
			memmove(&str[i], &str[i + 1], (len--) - i);
		else if (str[i] == 'n')
			str[i - 1] = '\n', memmove(&str[i], &str[i + 1], (len--) - i);
		else if (str[i] == 'r')
			str[i - 1] = '\r', memmove(&str[i], &str[i + 1], (len--) - i);
		else if (str[i] == 't')
			str[i - 1] = '\t', memmove(&str[i], &str[i + 1], (len--) - i);
	}
	return;
}


struct desktop_file_s *desktop_parse_append(char *str, struct desktop_file_s *df) {
	/* TODO: Make these dynamically reallocable */
	char key[4096], value[4096], buff[4096], buff2[4096], *tmp;
	int sz, brk, section = 0;

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
			section = desktop_section_new(df, buff);
		} else if (*key == ';' || *key == '#') {
		} else {
			desktop_entry_unescape(value);
			if (strchr(key, '[')) {	/* locale string */
				if (!strchr(key, ']'))
					continue;	/* nope.avi */
				*strchr(key, '[') = '\n', *strchr(key, ']') = 0;
				sscanf(key, "%[^\n]\n%[^\n]", buff, buff2);
				desktop_entry_new(df, buff, buff2, value, section);
			} else
				desktop_entry_new(df, key, "", value, section);
		}
	}
	
	return df;
}

struct desktop_file_s *desktop_parse(char *str) {
	struct desktop_file_s *df;

	df = malloc(sizeof(*df));
	df->sections = 0, df->section = NULL;
	desktop_section_new(df, NULL);
	return desktop_parse_append(str, df);
}


struct desktop_file_s *desktop_parse_file_append(const char *path, struct desktop_file_s *df) {
	FILE *fp;
	char *buff;
	long file_sz;

	if (!(fp = fopen(path, "r")))
		return NULL;
	fseek(fp, 0, SEEK_END);
	file_sz = ftell(fp);
	rewind(fp);
	buff = malloc(file_sz + 1);
	buff[fread(buff, 1, file_sz, fp)] = 0;
	fclose(fp);
	if (!df)
		df = desktop_parse(buff);
	else
		desktop_parse_append(buff, df);
	free(buff);
	return df;
}

struct desktop_file_s *desktop_parse_file(const char *path) {
	return desktop_parse_file_append(path, NULL);
}

static void desktop_write_format(FILE *fp, char *str) {
	int i, len;
	len = strlen(str);
	for (i = 0; i < len; i++) {
		if (str[i] == '\\')
			fprintf(fp, "\\\\");
		else if (str[i] == '\r')
			fprintf(fp, "\\r");
		else if (str[i] == '\n')
			fprintf(fp, "\\n");
		else if (str[i] == '\t')
			fprintf(fp, "\\t");
		else
			fwrite(&str[i], 1, 1, fp);
	}

	fprintf(fp, "\n");
}


void desktop_write(struct desktop_file_s *df, const char *path) {
	FILE *fp;
	int i, j;

	if (!df) return;

	if (!(fp = fopen(path, "w"))) {
		fprintf(stderr, "Unable to write %s\n", path);
		return;
	}
	for (i = 0; i < df->sections; i++) {
		if (df->section[i].name)
			fprintf(fp, "[%s]\n", df->section[i].name);
		for (j = 0; j < df->section[i].entries; j++) {
			if (!*df->section[i].entry[j].key)
				continue;
			else if (*df->section[i].entry[j].locale)
				fprintf(fp, "%s[%s]=", df->section[i].entry[j].key, df->section[i].entry[j].locale);
			else
				fprintf(fp, "%s=", df->section[i].entry[j].key);
			desktop_write_format(fp, df->section[i].entry[j].value);
		}
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
