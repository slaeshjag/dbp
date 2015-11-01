#include "dbp.h"
#include "config.h"
#include "desktop.h"
#include <stdlib.h>
#include <string.h>

struct DBPConfig dbp_config_struct;

char *dbp_config_version_get() {
	return VERSION;
}


static char *config_request_entry(struct DBPDesktopFile *df, const char *key) {
	const char *tmp = dbp_desktop_lookup(df, key, "", "Package Daemon Config");

	if (!tmp) {
		fprintf(dbp_error_log, "%s: key %s is missing\n", DBP_CONFIG_FILE_PATH, key);
		fflush(dbp_error_log);
		exit(-1);
	}

	return strdup(tmp);
}

void dbp_config_expand_token(char ***target, int *targets, char *token) {
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

static int config_get_bool(struct DBPDesktopFile *df, const char *key) {
	char *tmp;
	int ret;
	
	tmp = config_request_entry(df, key);
	if (!tmp)
		return 0;
	ret = !strcmp(tmp, "yes");
	free(tmp);
	return ret;
}


int dbp_config_init() {
	struct DBPDesktopFile *df;
	struct DBPConfig c;
	char *tmp;

	/* En fuling åt h4xxel's vala-jox */
	if (!dbp_error_log)
		dbp_error_log = stderr;

	c.file_extension = NULL, c.file_extensions = 0;

	if (!(df = dbp_desktop_parse_file(DBP_CONFIG_FILE_PATH))) {
		fprintf(dbp_error_log, "Unable to open config file %s\n", DBP_CONFIG_FILE_PATH);
		return 0;
	}

	c.file_extension = NULL, c.file_extensions = 0;
	tmp = config_request_entry(df, "file_extension");
	dbp_config_expand_token(&c.file_extension, &c.file_extensions, tmp);
	free(tmp);

	c.search_dir = NULL, c.search_dirs = 0;
	tmp = config_request_entry(df, "search_directories");
	dbp_config_expand_token(&c.search_dir, &c.search_dirs, tmp);
	free(tmp);

	c.img_mount = config_request_entry(df, "image_mount_dir");
	c.union_mount = config_request_entry(df, "union_mount_dir");

	c.data_directory = config_request_entry(df, "data_directory");
	c.rodata_directory = config_request_entry(df, "ro_data_directory");

	c.per_package_appdata = config_get_bool(df, "per_package_appdata");
	c.create_rodata = config_get_bool(df, "create_rodata");
	c.icon_directory = config_request_entry(df, "icon_directory");
	c.exec_directory = config_request_entry(df, "exec_directory");
	c.desktop_directory = config_request_entry(df, "desktop_directory");
	c.dbpout_directory = config_request_entry(df, "dbpout_directory");
	c.dbpout_prefix = config_request_entry(df, "dbpout_prefix");
	c.dbpout_suffix = config_request_entry(df, "dbpout_suffix");
	c.daemon_log = config_request_entry(df, "daemon_log");
	c.state_file = config_request_entry(df, "state_file");

	c.exec_template = config_request_entry(df, "exec_template");

	c.per_user_appdata = config_get_bool(df, "per_user_appdata");

	c.arch = NULL, c.archs = 0;
	tmp = config_request_entry(df, "supported_archs");
	dbp_config_expand_token(&c.arch, &c.archs, tmp);
	free(tmp);

	/* Optional key */
	c.run_script = dbp_desktop_lookup(df, "run_script", "", "Package Daemon Config");
	if (c.run_script)
		c.run_script = strdup(c.run_script);

	c.verbose_output = config_get_bool(df, "verbose_debug_output");

	c.df = df;

//	desktop_free(df);
	dbp_config_struct = c;
	return 1;
}
