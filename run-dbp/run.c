#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "loop.h"
#include "comm.h"
#include "dbp.h"
#include "config.h"
#include "desktop.h"

struct {
	char			*exec;
	char			*pkg_id;
	int			use_path;
} run_opt;


char *run_user_get() {
	char *buff;
	
	buff = malloc(64);
	getlogin_r(buff, 64);

	return buff;
}


int run_appdata_create(const char *pkg_id, const char *user) {
	char path[PATH_MAX];
	char *mount;
	int err;

	if ((err = comm_dbus_request_mountp("pyra-testpkg_slaeshjag", &mount)) < 0)
		return err;
	if (config_struct.per_user_appdata)
		sprintf(path, "%s/%s_%s/%s", mount, config_struct.data_directory, user, pkg_id);
	else
		sprintf(path, "%s/%s/%s", mount, config_struct.data_directory, pkg_id);
	free(mount);
	loop_directory_setup(path, 0755);

	/* TODO: Check that directory exists */
	return 0;
}


void run_parse_args(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <package-id> <executable> [--args]", argv[0]);
		exit(-1);
	}

	run_opt.pkg_id = argv[1];
	run_opt.exec = argv[2];
	run_opt.use_path = (!strstr(argv[0], "run-dbp-path"));

	return;
}


void run_exec() {
	char exec[PATH_MAX];

	sprintf(exec, "%s/%s/%s", config_struct.union_mount, run_opt.pkg_id, run_opt.exec);
	/* FIXME: Remove this hack and put something real here */
	system(exec);
	return;
}


void run_id(char *id, char *user) {
	int run_id;

	run_appdata_create(run_opt.pkg_id, user);
	run_id = comm_dbus_request_mount(id, user);
	run_exec();
	comm_dbus_request_umount(run_id);
	
	return;
}


char *run_locate_default_exec(const char *pkg_id) {
	char path[PATH_MAX], *exec;
	struct desktop_file_s *df;

	snprintf(path, PATH_MAX, "%s/%s%s_default.desktop", config_struct.desktop_directory,
	    DBP_META_PREFIX, pkg_id);
	if (!(df = desktop_parse_file(path)))
		return strdup("");
	if ((exec = desktop_lookup(df, "Exec", "", "Desktop Entry")))
		exec = strdup(exec);
	else
		exec = strdup("");
	desktop_free(df);
	return exec;
}


int run_path() {
	int ret, retret;
	char *id, *exec;

	ret = comm_dbus_register_path(run_opt.pkg_id, &id);
	if (ret < 0) {
		if (ret != DBP_ERROR_PKG_REG) {
			fprintf(stderr, "package registration failed %i\n", ret);
			return ret;
		}
	}

	fprintf(stderr, "Id is %s\n", id);
	retret = 0;
	exec = run_locate_default_exec(id);
	if (!strlen(exec)) {
		retret = DBP_ERROR_NO_DEFAULTH;
		goto cleanup;
	}

	fprintf(stderr, "Default exec is %s\n", exec);

	cleanup:
	free(exec);
	if (ret >= 0)
		comm_dbus_unregister_path(run_opt.pkg_id);

	return retret;
}


int main(int argc, char **argv) {
	char *user;

	run_parse_args(argc, argv);
	config_init();
	comm_dbus_init();

	user = run_user_get();
	if (run_opt.use_path)
		run_id(run_opt.pkg_id, user);
	else
		run_path();

	free(user);
	return 0;
}
