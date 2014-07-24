#include "config.h"
#include "desktop.h"
#include <stdlib.h>
#include <string.h>

struct config_s config_struct;


static void config_expand_token(char ***target, int *targets, char *token) {
	char *saveptr;
	int i;

	if (!token)
		return;
	for (i = 0, token = strtok_r(token, ";", &saveptr); token; token = strtok_r(NULL, ";", &saveptr), i++) {
		*target = realloc(*target, sizeof(**target) * (i + 1));
		(*target)[i] = strdup(token);
	}
	
	*targets = i;
	return;
}


void config_init() {
	struct desktop_file_s *df;
	struct config_s c;
	char *tmp;

	c.file_extension = NULL, c.file_extensions = 0;

	if (!(df = desktop_parse_file(CONFIG_FILE_PATH))) {
		fprintf(stderr, "Unable to open config file %s\n", CONFIG_FILE_PATH);
		return;
	}

	c.file_extension = NULL, c.file_extensions = 0;
	tmp = desktop_lookup(df, "file_extension", "", "Package Daemon Config");
	config_expand_token(&c.file_extension, &c.file_extensions, tmp);

	c.search_dir = NULL, c.search_dirs = 0;
	tmp = desktop_lookup(df, "search_directories", "", "Package Daemon Config");
	config_expand_token(&c.search_dir, &c.search_dirs, tmp);

	desktop_free(df);
	config_struct = c;
	return;
}
