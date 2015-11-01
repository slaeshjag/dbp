#define	_LIBINTERNAL
#include "loop.h"
#include "util.h"
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
#include <assert.h>
#include <ctype.h>


int dbp_loop_desktop_directory(const char *path) {
	if (strstr(path, "desktop/"))
		if (!strchr(strstr(path, "desktop/") + strlen("desktop/"), '/'))
			return 1;
	return 0;
}


int dbp_loop_directory_setup(const char *path, int umask) {
	char *path_tok, *tmp, *saveptr, *next_path = NULL, *old_path = NULL;
	struct stat dir_stat;

	assert(path);

	if (!(path_tok = strdup(path)))
		goto fail;

	while ((tmp = strstr(path_tok, "//")))
		memmove(tmp, tmp + 1, strlen(tmp));

	old_path = NULL;
	for (tmp = strtok_r(path_tok, "/", &saveptr); tmp; tmp = strtok_r(NULL, "/", &saveptr)) {
		if (!(next_path = dbp_string("%s/%s", (old_path ? old_path : ""), tmp)))
			goto fail;

		if (!stat(next_path, &dir_stat)) {
			if (!S_ISDIR(dir_stat.st_mode))
				goto exists_but_not_a_directory;
		} else {
			if (mkdir(next_path, umask))
				goto unable_to_create_directory;
			chmod(next_path, umask);
		}

		free(old_path);
		old_path = next_path;
	}

	if (old_path != next_path)
		free(old_path);

	free(next_path);
	free(path_tok);
	return 1;

exists_but_not_a_directory:
	fprintf(dbp_error_log, "%s exists but isn't a directory\n", next_path);
	goto fail;
unable_to_create_directory:
	fprintf(dbp_error_log, "Unable to create directory %s\n", next_path);
fail:
	free(path_tok);
	free(old_path);
	free(next_path);
	return 0;
}


static int loop_available_check(int i) {
	int loop_fd = -1;
	uint64_t sz;
	char *find;

	if (!(find = dbp_string("/dev/loop%i", i)))
		goto fail;

	if ((loop_fd = open(find, O_RDWR)) < 0)
		goto fail;

	free(find);
	find = NULL;

	if (ioctl(loop_fd, BLKGETSIZE64, &sz) < 0)
		goto fail;

	close(loop_fd);
	return (sz > 0 ? 0 : 1);

fail:
	if (loop_fd >= 0)
		close(loop_fd);
	free(find);
	return 0;
}


/* Locate a loop device that isn't in use */
static int loop_available_get() {
	DIR *d;
	struct dirent de, *result;
	int n;

	if (!(d = opendir("/dev")))
		return -1;

	readdir_r(d, &de, &result);
	for (n = -1; result; n = -1) {
		if (strstr(de.d_name, "loop") == de.d_name) {
			sscanf(de.d_name, "loop%i", &n);
			if (n >= 0 && loop_available_check(n))
				break;
		}
		readdir_r(d, &de, &result);
	}

	closedir(d);
	return n;
}


static int loop_setup(const char *image, int loop_n) {
	int img_fd = -1, loop_fd = -1;
	char *device = NULL;
	int ret = 0;

	assert(image);

	if ((img_fd = open(image, O_RDWR)) < 0)
		goto no_access;

	if (!(device = dbp_string("/dev/loop%i", loop_n)))
		goto fail;

	if ((loop_fd = open(device, O_RDWR)) < 0)
		goto open_fail;

	free(device);
	device = NULL;

	if (ioctl(loop_fd, LOOP_SET_FD, img_fd) < 0)
		goto loop_set_fail;

	close(img_fd);
	close(loop_fd);
	return 1;

no_access:
	ret = DBP_ERROR_NO_PKG_ACCESS;
	goto fail;
open_fail:
	fprintf(dbp_error_log, "failed to open: %s\n", device);
	goto fail;
loop_set_fail:
	fprintf(dbp_error_log, "%s\n", strerror(errno));
	ret = DBP_ERROR_SET_LOOP2;
fail:
	free(device);
	if (img_fd >= 0)
		close(img_fd);
	if (loop_fd >= 0)
		close(loop_fd);
	return ret;
}


static int loop_reset(int loop) {
	char *device;
	int loop_fd;

	if (!(device = dbp_string("/dev/loop%i", loop)))
		goto fail;

	if ((loop_fd = open(device, O_RDWR)) < 0)
		goto fail;

	free(device);
	device = NULL;

	if (ioctl(loop_fd, LOOP_CLR_FD) < 0) {
		close(loop_fd);
		goto fail;
	}
	close(loop_fd);
	return 1;

fail:
	free(device);
	return 0;
}

static void loop_decide_appdata(int fs, const char *appdata, const char *fs_path, char **user_dir, char **rodata, const char *user) {
	// Only the user and rodata dirs are outside of global system configuration. Only these needs to be escaped, as
	// it can be assumed that global system configuration and pkgid validation will keep the rest of the paths valid.

	if (dbp_config_struct.per_user_appdata) {
		if (dbp_config_struct.per_package_appdata) {
			*user_dir = dbp_string("%s/%s_%s/%s", fs?fs_path:"", dbp_config_struct.data_directory, user, appdata);
			*rodata = dbp_string("%s/%s_%s/%s", fs?fs_path:"", dbp_config_struct.rodata_directory, user, appdata);
		} else {
			*user_dir = dbp_string("%s/%s_%s", fs?fs_path:"", dbp_config_struct.data_directory, user);
			*rodata = dbp_string("%s/%s_%s", fs?fs_path:"", dbp_config_struct.rodata_directory, user);
		}
	} else {
		if (dbp_config_struct.per_package_appdata) {
			*user_dir = dbp_string("%s/%s/%s", fs?fs_path:"", dbp_config_struct.data_directory, appdata);
			*rodata = dbp_string("%s/%s/%s", fs?fs_path:"", dbp_config_struct.rodata_directory, appdata);
		} else {
			*user_dir = dbp_string("%s/%s", fs?fs_path:"", dbp_config_struct.data_directory);
			*rodata = dbp_string("%s/%s", fs?fs_path:"", dbp_config_struct.rodata_directory);
		}
	}
}


int dbp_loop_mount(const char *image, const char *id, const char *user, const char *src_mount, const char *appdata) {
	int loop_n = -1, ret = 0, lret, rodata = 0;
	char *mount_path = NULL, *loop = NULL, *user_dir = NULL, *img_dir = NULL, *rodata_dir = NULL;
	char *mount_opt = NULL;
	struct stat st;

	assert(image && id && user && src_mount && appdata);

	if (strlen(user) > 63)	/* This is not a reasonable user name */
		return DBP_ERROR_INTERNAL_MSG;

	if ((loop_n = loop_available_get()) < 0)
		goto no_loop;

	if ((lret = loop_setup(image, loop_n)) < 0)
		goto loop_set_fail;

	if (!(img_dir = dbp_string("%s/%s", dbp_config_struct.img_mount, id)))
		goto fail;

	if (!(loop = dbp_string("/dev/loop%i", loop_n)))
		goto fail;

	/* It'll mount read-only, as it's an image */
	dbp_loop_directory_setup(img_dir, 0555);
	if (mount(loop, img_dir, DBP_FS_NAME, 0, ""))
		goto bad_fsimg;

	free(loop);
	loop = NULL;

	/***** Mount the UnionFS *****/
	if (!(mount_path = dbp_string("%s/%s", dbp_config_struct.union_mount, id)))
		goto fail;

	dbp_loop_directory_setup(mount_path, 0555);
	loop_decide_appdata(src_mount[1]!=0, appdata, src_mount, &user_dir, &rodata_dir, user);

	if (!user_dir || !rodata_dir)
		goto fail;

	if (strchr(user_dir, ':') || strchr(img_dir, ':') || strchr(rodata_dir, ':'))
		goto illegal_dirname;
	if (strchr(user_dir, '=') || strchr(img_dir, '=') || strchr(rodata_dir, '='))
		goto illegal_dirname;
	if (strchr(user_dir, ',') || strchr(img_dir, ',') || strchr(rodata_dir, ','))
		goto illegal_dirname;
	
	if (!stat(rodata_dir, &st))
		if (S_ISDIR(st.st_mode))
			rodata = 1;
	/* FIXME: nolwh, noplink should really only be used when vfat is detected */
	if (!rodata)
		if (!(mount_opt = dbp_string("noplink,br=%s=rw+nolwh:%s=ro", user_dir, img_dir)))
			goto fail;
	if (rodata)
		if (!(mount_opt = dbp_string("noplink,br=%s=rw+nolwh:%s=ro+nolwh:%s=ro", user_dir, rodata_dir, img_dir)))
			goto fail;

	free(user_dir), user_dir = NULL;
	free(rodata_dir), rodata_dir = NULL;

	if (mount("none", mount_path, DBP_UNIONFS_NAME, 0, mount_opt) < 0)
		goto union_failed;

	free(img_dir), img_dir = NULL;
	free(mount_path), mount_path = NULL;
	free(mount_opt);
	return loop_n;

no_loop:
	ret = DBP_ERROR_NO_LOOP;
	goto fail;
loop_set_fail:
	ret = lret;
	goto fail;
bad_fsimg:
	ret = DBP_ERROR_BAD_FSIMG;
	goto fail;
illegal_dirname:
	ret = DBP_ERROR_ILL_DIRNAME;
	goto fail;
union_failed:
	fprintf(dbp_error_log, "%s, %s\n", mount_opt, strerror(errno));
	umount(img_dir);
	ret = DBP_ERROR_UNION_FAILED;
fail:
	free(img_dir);
	free(rodata_dir);
	free(loop);
	free(mount_path);
	free(user_dir);
	free(mount_opt);
	if (loop_n >= 0)
		loop_reset(loop_n);
	return ret;
}


int dbp_loop_umount(const char *pkg_id, int loop, const char *user, int prev_state) {
	char *mount_path, *img_path = NULL;

	/* parameter user will eventually be used */
	(void) user;
	assert(pkg_id);

	if (!(mount_path = dbp_string("%s/%s", dbp_config_struct.union_mount, pkg_id)))
		goto fail;

	if (!(img_path = dbp_string("%s/%s", dbp_config_struct.img_mount, pkg_id)))
		goto fail;

	if (prev_state < -1)
		goto img_path;
	prev_state = prev_state<0?prev_state:1;	// If this fails, we could still use the mount
	if (umount(mount_path) < 0)
		goto fail;
	img_path:
	if (prev_state < -2)
		goto loop;
	prev_state = -2;
	if (umount(img_path) < 0)
		goto fail;

	rmdir(mount_path);
	rmdir(img_path);

	loop:
	prev_state = -3;
	if (!loop_reset(loop))
		goto fail;

	free(mount_path);
	free(img_path);

	return 0;

fail:
	free(mount_path);
	free(img_path);
	return prev_state;
}
