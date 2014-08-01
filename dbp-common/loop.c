#include "loop.h"
#include "dbp.h"
#include "config.h"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/loop.h>
#include <limits.h>
#include <errno.h>


int loop_directory_setup(const char *path, int umask) {
	char *path_tok, *tmp, *saveptr, next_path[PATH_MAX];
	struct stat dir_stat;

	path_tok = strdup(path);
	while ((tmp = strstr(path_tok, "//")))
		memmove(tmp, tmp + 1, strlen(tmp));

	sprintf(next_path, "/");
	for (tmp = strtok_r(path_tok, "/", &saveptr); tmp; tmp = strtok_r(NULL, "/", &saveptr)) {
		strcat(next_path, tmp);
		if (!stat(next_path, &dir_stat)) {
			if (!S_ISDIR(dir_stat.st_mode)) {
				fprintf(dbp_error_log, "%s exists but isn't a directory\n", next_path);
				free(path_tok);
				return 0;
			}
		} else {
			if (mkdir(next_path, umask)) {
				fprintf(dbp_error_log, "Unable to create directory %s\n", next_path);
				free(path_tok);
				return 0;
			}
			chmod(next_path, umask);
		}
		strcat(next_path, "/");
	}

	free(path_tok);
	return 1;
}
		

static int loop_available_check(int i) {
	int loop_fd;
	uint64_t sz;
	char find[32];

	sprintf(find, "/dev/loop%i", i);
	if ((loop_fd = open(find, O_RDWR)) < 0)
		return 0;
	
	if (ioctl(loop_fd, BLKGETSIZE64, &sz) < 0) {
		close(loop_fd);
		return 0;
	}

	close(loop_fd);
	if (sz > 0)
		return 0;
	return 1;
}


/* Locate a loop device that isn't in use */
static int loop_available_get() {
	DIR *d;
	struct dirent de, *result;
	int n;

	if (!(d = opendir("/dev")))
		return 0;
	readdir_r(d, &de, &result);
	while (result) {
		n = -1;
		if (strstr(de.d_name, "loop") == de.d_name) {
			sscanf(de.d_name, "loop%i", &n);
			if (n < 0);
			else if (loop_available_check(n))
				goto done;
		}
		readdir_r(d, &de, &result);
	}
	
	n = -1;

	done:
	closedir(d);
	return n;
}


static int loop_setup(const char *image, int loop_n) {
	int img_fd, loop_fd;
	char device[32];

	if ((img_fd = open(image, O_RDWR)) < 0)
		return DBP_ERROR_NO_PKG_ACCESS;
	sprintf(device, "/dev/loop%i", loop_n);
	if ((loop_fd = open(device, O_RDWR)) < 0) {
		close(img_fd);
		return DBP_ERROR_SET_LOOP;
	}

	/* TODO: Remove this hack and free the loop properly instead */
	ioctl(loop_fd, LOOP_CLR_FD, 0);

	if (ioctl(loop_fd, LOOP_SET_FD, img_fd) < 0) {
		close(img_fd);
		close(loop_fd);
		fprintf(dbp_error_log, "%s\n", strerror(errno));
		return DBP_ERROR_SET_LOOP2;
	}

	close(img_fd);
	close(loop_fd);
	return 1;
}


static void loop_reset(int loop) {
	char device[32];
	int loop_fd;

	sprintf(device, "/dev/loop%i", loop);
	if ((loop_fd = open(device, O_RDWR)) < 0)
		return;
	ioctl(loop_fd, LOOP_CLR_FD);
	close(loop_fd);
	return;
}

	
int loop_mount(const char *image, const char *id, const char *user, const char *src_mount, const char *appdata) {
	int loop_n, ret;
	char mount_path[PATH_MAX], loop[32], user_dir[PATH_MAX], img_dir[PATH_MAX];
	char *mount_opt;

	if (strlen(user) > 63)	/* This is not a reasonable user name */
		return 0;
	if (strlen(user) + strlen(src_mount) + strlen(config_struct.data_directory) + strlen(id) + 4 >= PATH_MAX)
		return 0;

	if ((loop_n = loop_available_get()) < 0)
		return DBP_ERROR_NO_LOOP;
	if ((ret = loop_setup(image, loop_n)) < 0)
		return ret;
	sprintf(img_dir, "%s/%s", config_struct.img_mount, id);
	sprintf(loop, "/dev/loop%i", loop_n);

	/* It'll mount read-only, as it's an image */
	loop_directory_setup(img_dir, 0555);
	if (mount(loop, img_dir, DBP_FS_NAME, 0, "")) {
		loop_reset(loop_n);
		return DBP_ERROR_BAD_FSIMG;
	}

	/***** Mount the UnionFS *****/
	sprintf(mount_path, "%s/%s", config_struct.union_mount, id);
	loop_directory_setup(mount_path, 0555);
	if (config_struct.per_user_appdata) {
		/* If user tries to escape the dir, that's no big deal.	*
		** There will be no privilegie escelation, and it's up	*
		** to the user package runner to create it		*/
		if (src_mount[1])
			sprintf(user_dir, "%s/%s_%s/%s", src_mount, config_struct.data_directory, user, appdata);
		else
			sprintf(user_dir, "/%s_%s/%s", config_struct.data_directory, user, appdata);
	} else {
		if (src_mount[1])
			sprintf(user_dir, "%s/%s/%s", src_mount, config_struct.data_directory, appdata);
		else
			sprintf(user_dir, "/%s/%s", config_struct.data_directory, appdata);
	}
	if (strchr(user_dir, ':') || strchr(img_dir, ':')) {
		umount(img_dir);
		loop_reset(loop_n);
		return DBP_ERROR_ILL_DIRNAME;
	}

	mount_opt = malloc(strlen(user_dir) + strlen(img_dir) + strlen("br=") + 2);
	sprintf(mount_opt, "br=%s:%s", user_dir, img_dir);
	if (mount("none", mount_path, DBP_UNIONFS_NAME, 0, mount_opt) < 0) {
		fprintf(dbp_error_log, "%s, %s\n", mount_opt, strerror(errno));
		free(mount_opt);
		umount(img_dir);
		loop_reset(loop_n);
		return DBP_ERROR_UNION_FAILED;
	}

	free(mount_opt);
	
	return loop_n;
}


void loop_umount(const char *pkg_id, int loop, const char *user) {
	char mount_path[PATH_MAX], img_path[PATH_MAX];

	sprintf(mount_path, "%s/%s", config_struct.union_mount, pkg_id);
	sprintf(img_path, "%s/%s", config_struct.img_mount, pkg_id);

	umount(mount_path);
	umount(img_path);

	rmdir(mount_path);
	rmdir(img_path);
	loop_reset(loop);

	return;
}
