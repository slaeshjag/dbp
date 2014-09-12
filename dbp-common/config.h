#ifndef __CONFIG_H__
#define	__CONFIG_H__

#define	CONFIG_FILE_PATH	"/tmp/dbp_config.ini"

struct config_s {
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
};


int config_init();
extern struct config_s config_struct;
void config_expand_token(char ***target, int *targets, char *token);

#endif
