#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <dbpbase/dbpbase.h>
#include "mountwatch.h"
#include "package.h"
#include "comm.h"
#include "state.h"

static struct package_s *p_s;

static void sleep_usec(int usec) {
	#if 1
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = usec * 1000;
	nanosleep(&ts, NULL);
	#else
	usleep(usec);
	#endif
	return;
}


/* Responisble for saving all state when it's time to shut down */
void shutdown(int signal) {
	((void) signal);

	fprintf(dbp_error_log, "Killing mountwatch...\n");
	mountwatch_kill();
	/* Wait for mountwatch threads to die */
	while (!mountwatch_died())
		sleep_usec(1000);
	fprintf(dbp_error_log, "Killing dbus...\n");
	comm_kill();
	/* Wait for dbus thread to die */
	while (!comm_died())
		sleep_usec(1000);
	fprintf(dbp_error_log, "Dumping state...\n");
	state_dump(p_s);
	exit(0);
}


static int daemon_nuke_dir(char *dir) {
	DIR *d;
	struct dirent de, *result;
	char path[PATH_MAX];
	
	if (!(d = opendir(dir)))
		return 0;
	for (readdir_r(d, &de, &result); result; readdir_r(d, &de, &result))
		if (strstr(de.d_name, DBP_META_PREFIX) == de.d_name)
			snprintf(path, PATH_MAX, "%s/%s", dir, de.d_name), unlink(path);
	closedir(d);
	return 1;
}


static int daemon_nuke_execs() {
	DIR *d;
	struct dirent de, *result;
	FILE *fp;
	char buff[512], path[PATH_MAX];

	if (!(d = opendir(dbp_config_struct.exec_directory)))
		return 0;
	for (readdir_r(d, &de, &result); result; readdir_r(d, &de, &result)) {
		snprintf(path, PATH_MAX, "%s/%s", dbp_config_struct.exec_directory, de.d_name);
		if (!(fp = fopen(path, "r")))
			continue;
		*buff = 0;
		fgets(buff, 512, fp);
		fgets(buff, 512, fp);
		fclose(fp);
		buff[511] = 0;
		if (strcmp(buff, "#dbp-template\n"))
			continue;
		unlink(path);
	}

	closedir(d);

	return 1;
}


static int daemon_nuke() {
	if (!daemon_nuke_dir(dbp_config_struct.desktop_directory));
	else if (!daemon_nuke_dir(dbp_config_struct.icon_directory));
	else if (!daemon_nuke_execs());
	else
		return 1;
	return 0;
}


static int daemon_init() {
	if (!dbp_loop_directory_setup(dbp_config_struct.img_mount, 0755));
	else if (!dbp_loop_directory_setup(dbp_config_struct.union_mount, 0755));
	else if (!dbp_loop_directory_setup(dbp_config_struct.icon_directory, 0755));
	else if (!dbp_loop_directory_setup(dbp_config_struct.exec_directory, 0755));
	else if (!dbp_loop_directory_setup(dbp_config_struct.desktop_directory, 0755));
	else if (!dbp_loop_directory_setup(dbp_config_struct.dbpout_directory, 0777));
	else if (!daemon_nuke());
	else
		return 1;
	return 0;
}


static void create_pidfile() {
	FILE *fp;

	if ((fp = fopen("/var/run/dbpd.pid", "w"))) {
		fprintf(fp, "%i", getpid());
		fclose(fp);
	}

	return;
}

void die(int signal) {
	if (signal == SIGALRM)
		exit(1);
	exit(0);
}


int main(int argc, char **argv) {
	struct mountwatch_change_s change;
	struct package_s p;
	int i, sig_parent = 0;
	char *n;
	pid_t procid;

	p_s = &p;
	if (!dbp_init(NULL))
		return -1;
	if (!(dbp_error_log = fopen(dbp_config_struct.daemon_log, "w"))) {
		dbp_error_log = stderr;
		fprintf(stderr, "Unable to open %s\n", dbp_config_struct.daemon_log);
	} else
		setbuf(dbp_error_log, NULL);

	fprintf(dbp_error_log, "DBPd version %s starting...\n", dbp_config_version_get());
	p = package_init();
	state_recover(&p);

	if (argc > 1 && !strcmp(argv[1], "-d")) {	/* Daemonize */
		sig_parent = 1;
		if ((procid = fork()) < 0) {
			fprintf(stderr, "Unable to fork();\n");
			return 1;
		}

		if (procid) {
			/* Make sure we don't get stuck forever... */
			signal(SIGINT, die);
			signal(SIGALRM, die);
			alarm(5);
			pause();
			exit(0);
		}

		umask(0);
		if (setsid() < 0) {
			fprintf(stderr, "Unable to setsid()\n");
			exit(1);
		}

		chdir("/");
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		create_pidfile();
	}
	
	comm_dbus_register(&p);
	if (sig_parent)
		kill(getppid(), SIGINT);

	/* TODO: Replace with sigaction */
	signal(SIGTERM, shutdown);
	signal(SIGINT, shutdown);
	signal(SIGHUP, shutdown);
	signal(SIGUSR1, shutdown);
	signal(SIGUSR2, shutdown);

	if (!mountwatch_init())
		exit(-1);
	if (!daemon_init())
		exit(-1);
	
	for (;;) {
		change = mountwatch_diff();
		for (i = 0; i < change.entries; i++) {
			switch (change.entry[i].tag) {
				case MOUNTWATCH_TAG_REMOVED:
					package_release_mount(&p, change.entry[i].device);
					break;
				case MOUNTWATCH_TAG_ADDED:
					package_crawl_mount(&p, change.entry[i].device, change.entry[i].mount);
					break;
				case MOUNTWATCH_TAG_PKG_ADDED:
					package_register_path(&p, change.entry[i].device, change.entry[i].path, change.entry[i].mount, &n);
					free(n);
					break;
				case MOUNTWATCH_TAG_PKG_REMOVED:
					package_release_path(&p, change.entry[i].path);
					break;
				default:
					break;
			}
		}

		mountwatch_change_free(change);
		package_purgatory_check(&p);
	}

	(void) argc, (void) argv;

	return 0;
}
