#ifndef __DBP_CONFIG_H__
#define	__DBP_CONFIG_H__

#ifdef _LIBINTERNAL
#include "desktop.h"
#endif
#define	DBP_CONFIG_FILE_PATH	"/etc/dbp/dbp_config.ini"

struct DBPConfig {
	char			**file_extension;
	int			file_extensions;
	char			**search_dir;
	int			search_dirs;
	char			*img_mount;
	char			*union_mount;

	char			*data_directory;
	char			*rodata_directory;
	char			*icon_directory;
	char			*exec_directory;
	char			*desktop_directory;

	char			*dbpout_directory;
	char			*dbpout_prefix;
	char			*dbpout_suffix;

	char			*daemon_log;

	char			*exec_template;

	int			per_user_appdata;
	int			per_package_appdata;
	int			create_rodata;

	char			*state_file;
	char			*run_script;

	char			**arch;
	int			archs;

	int			verbose_output;

	struct DBPDesktopFile	*df;
};


int dbp_config_init();
extern struct DBPConfig dbp_config_struct;
char *dbp_config_version_get();
void dbp_config_expand_token(char ***target, int *targets, char *token);

#endif
