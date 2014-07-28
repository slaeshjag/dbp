#include "config.h"
#include "desktop.h"
#include <stdlib.h>
#include <string.h>

struct config_s config_struct;


void config_expand_token(char ***target, int *targets, char *token) {
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

	c.img_mount = strdup(desktop_lookup(df, "image_mount_dir", "", "Package Daemon Config"));
	c.union_mount = strdup(desktop_lookup(df, "union_mount_dir", "", "Package Daemon Config"));

	c.data_directory = strdup(desktop_lookup(df, "data_directory", "", "Package Daemon Config"));
	c.icon_directory = strdup(desktop_lookup(df, "icon_directory", "", "Package Daemon Config"));
	c.exec_directory = strdup(desktop_lookup(df, "exec_directory", "", "Package Daemon Config"));
	c.desktop_directory = strdup(desktop_lookup(df, "desktop_directory", "", "Package Daemon Config"));
	c.exec_template = strdup(desktop_lookup(df, "exec_template", "", "Package Daemon Config"));

	c.per_user_appdata = (!strcmp(desktop_lookup(df, "per_user_appdata", "", "Package Daemon Config"), "yes"));

	desktop_free(df);
	config_struct = c;
	return;
}
