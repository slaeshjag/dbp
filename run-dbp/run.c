#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "loop.h"
#include "comm.h"
#include "dbp.h"
#include "config.h"
#include "desktop.h"

#define	FD_SET_NO_BLOCK(fd) 	(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK))

FILE *dbp_error_log;

struct {
	char			*exec;
	char			*pkg_id;
	char			**argv;
	int			use_path;
	int			env;
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
	int i, path = 0;

	if (!strstr(argv[0], "run-dbp-path") && argc < 3) {
		fprintf(stderr, "Usage: %s <package-id> <executable> [--env] [--args]\n", argv[0]);
		exit(-1);
	} else if (strstr(argv[0], "run-dbp-path") && argc < 2) {
		fprintf(stderr, "Usage: %s <path to package> [arguments to exec]\n", argv[0]);
		exit(-1);
	}

	run_opt.env = 0;
	if (strstr(argv[0], "run-dbp-path"))
		path = 1, run_opt.env = 1;

	for (i = 2; i < argc && !path; i++)
		if (!strcmp(argv[i], "--args")) {
			i++;
			break;
		} else if (!strcmp(argv[i], "--env"))
			run_opt.env = 1;
			
	run_opt.argv = &argv[i];
	run_opt.pkg_id = argv[1];
	run_opt.exec = argv[2];
	run_opt.use_path = !(!strstr(argv[0], "run-dbp-path"));

	return;
}


int run_exec(const char *exec, char **argv, int env, const char *log_file) {
	int pipeout[2], pipeerr[2];
	int pid, sz, outa, erra, max, log_fd;
	siginfo_t info;
	char buff[512];
	fd_set set;

	if (env)
		pipe(pipeout), pipe(pipeerr), log_fd = open(log_file, O_RDWR);
		
	pid = fork();
	
	if (!pid) {
		if (env) {
			dup2(pipeout[1], STDOUT_FILENO), dup2(pipeerr[1], STDERR_FILENO);
			close(pipeout[0]), close(pipeerr[0]);
		}
	
		execvp(exec, argv);
		exit(-1);
	} else {
		if (env) {
			close(pipeout[1]), close(pipeerr[1]);
			FD_SET_NO_BLOCK(pipeout[0]), FD_SET_NO_BLOCK(pipeerr[0]);
			outa = erra = 1;
			max = ((pipeout[1] > pipeerr[1]) ? pipeout[1] : pipeerr[1]) + 1;
		} else
			outa = erra = 0;

		while (outa || erra) {
			FD_ZERO(&set);
			if (outa) FD_SET(pipeout[0], &set);
			if (erra) FD_SET(pipeerr[0], &set);
			select(max, &set, NULL, NULL, NULL);

			if (FD_ISSET(pipeout[0], &set)) {
				sz = read(pipeout[0], buff, 512);
				write(STDOUT_FILENO, buff, sz);
				if (sz <= 0) outa = 0;
				write(log_fd, buff, sz);
			} if (FD_ISSET(pipeerr[0], &set)) {
				sz = read(pipeerr[0], buff, 512);
				write(STDERR_FILENO, buff, sz);
				if (sz <= 0) erra = 0;
				write(log_fd, buff, sz);
			}
		}

		if (env) close(log_fd);
		/* Wait for process to die */
		waitid(P_PID, pid, &info, WEXITED);
		if (info.si_code != CLD_EXITED) {
			if (info.si_code == CLD_KILLED && info.si_status == SIGINT)
				return -1;
			fprintf(stderr, "===== Abnormal termination =====\n");
			if (info.si_code == CLD_KILLED) {
				fprintf(stderr, "Process was killed by signal %i (%s)\n", info.si_status, strsignal(info.si_status));
				if (info.si_status == SIGSEGV)
					return DBP_ERROR_SIGSEGV;
				return DBP_ERROR_SIGEXIT;
			}
			return DBP_ERROR_MYSTKILL;
		}
	}

	return info.si_status;
}


void run_exec_prep() {
	char exec[PATH_MAX], path[PATH_MAX];

	snprintf(exec, PATH_MAX, "%s/%s/%s", config_struct.union_mount, run_opt.pkg_id, run_opt.exec);
	snprintf(path, PATH_MAX, "%s/%s%s%s", config_struct.dbpout_directory, config_struct.dbpout_prefix,
	    run_opt.pkg_id, config_struct.dbpout_suffix);
	/* FIXME: Remove this hack and put something real here */
	run_exec(exec, run_opt.argv, run_opt.env, path);
	return;
}


void run_id(char *id, char *user) {
	int run_id;
	char *appdata;

	comm_dbus_get_appdata(run_opt.pkg_id, &appdata);
	run_appdata_create(appdata, user);
	run_id = comm_dbus_request_mount(id, user);
	run_exec_prep();
	comm_dbus_request_umount(run_id);
	
	return;
}


void run_expand_arguments(const char *args) {
	char **newarg = NULL;
	int newargs, quote1, quote2, i, pos, skip;

	newargs = 1;
	newarg = malloc(sizeof(*newarg));
	newarg[newargs - 1] = malloc(strlen(args) + 1);
	for (i = pos = quote1 = quote2 = 0; args[i]; i++) {
		skip = 0;
		if (args[i] == '\'')
			quote1 = !quote1, skip = 1;
		if (args[i] == '\"')
			quote2 = !quote2, skip = 1;
		if (!quote1 && !quote2 && (args[i] == ' ' || args[i] == '\t')) {
			newarg[newargs - 1][pos] = 0;
			newargs++;
			newarg = realloc(newarg, sizeof(*newarg) * newargs);

			/* Wasting RAM like an Emacs! */
			newarg[newargs - 1] = malloc(strlen(&args[i]) + 1);
			pos = 0;
			continue;
		} else if (!skip)
			newarg[newargs - 1][pos++] = args[i];
	}

	newarg[newargs - 1][pos] = 0;

	for (i = 0; run_opt.argv[i]; i++);
	newarg = realloc(newarg, sizeof(*newarg) * (newargs + i + 1));
	for (i = 0; run_opt.argv[i]; i++)
		newarg[newargs + i] = strdup(run_opt.argv[i]);
	newarg[newargs + i + 1] = NULL;
	run_opt.argv = newarg;
	run_opt.exec = newarg[0];

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
	run_expand_arguments(exec);
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

	retret = 0;
	exec = run_locate_default_exec(id);
	if (!strlen(exec)) {
		retret = DBP_ERROR_NO_DEFAULTH;
		goto cleanup;
	}

	run_exec(run_opt.exec, run_opt.argv, 0, NULL);

	cleanup:
	free(exec);
	if (ret >= 0)
		comm_dbus_unregister_path(run_opt.pkg_id);

	return retret;
}


void run_inject_exec() {
	int i;
	char **tmp;

	for (i = 0; run_opt.argv[i]; i++);
	tmp = malloc(sizeof(*tmp) * (i + 1));
	tmp[0] = run_opt.exec;
	for (i = 0; run_opt.argv[i]; i++)
		tmp[i + 1] = run_opt.argv[i];
	run_opt.argv = tmp;
	return;
}


int main(int argc, char **argv) {
	char *user;
	dbp_error_log = stderr;

	run_parse_args(argc, argv);
	config_init();
	comm_dbus_init();

	user = run_user_get();
	if (!run_opt.use_path)
		run_inject_exec(), run_id(run_opt.pkg_id, user);
	else
		run_path();

	free(user);
	return 0;
}
