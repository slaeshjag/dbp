#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "loop.h"
#include "comm.h"
#include "dbp.h"
#include "config.h"

struct {
	char			*exec;
	char			*pkg_id;
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

	return;
}


void run_exec() {
	char exec[PATH_MAX];

	sprintf(exec, "%s/%s/%s", config_struct.union_mount, run_opt.pkg_id, run_opt.exec);
	/* FIXME: Remove this hack and put something real here */
	system(exec);
	return;
}


int main(int argc, char **argv) {
	char *user;
	int run_id;

	run_parse_args(argc, argv);
	config_init();
	comm_dbus_init();

	user = run_user_get();
	run_appdata_create(run_opt.pkg_id, user);
	run_id = comm_dbus_request_mount(run_opt.pkg_id, user);
	run_exec();
	comm_dbus_request_umount(run_id);
	free(user);
	return 0;
}
