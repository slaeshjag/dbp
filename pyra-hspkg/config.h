#ifndef __CONFIG_H__
#define	__CONFIG_H__

#define	CONFIG_FILE_PATH	"/tmp/dbp_config.ini"

struct config_s {
	char			**file_extension;
	int			file_extensions;
	char			**search_dir;
	int			search_dirs;
};


void config_init();
extern struct config_s config_struct;

#endif
