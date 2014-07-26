#include "loop.h"
#include "dbp.h"
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/loop.h>


static int loop_available_check(int i) {
	char path[32], find[32], buff[4096];
	FILE *fp;

	sprintf(find, "/dev/loop%i", i);
	if (!(fp = fopen("/proc/mounts", "r")))
		return 0;
	while (!feof(fp)) {
		fgets(buff, 4096, fp);
		buff[31] = 0;
		sscanf(buff, "%s \n", path);
		if (!strcmp(find, path)) {
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
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
			if (n < 0)
				continue;
			if (loop_available_check(n))
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

	if (ioctl(loop_fd, LOOP_SET_FD, img_fd) < 0) {
		close(img_fd);
		close(loop_fd);
		return DBP_ERROR_SET_LOOP2;
	}

	close(img_fd);
	close(loop_fd);
	return 1;
}

	
int loop_mount(const char *image) {
	int loop_n, ret;

	if ((loop_n = loop_available_get()) < 0)
		return DBP_ERROR_NO_LOOP;
	if ((ret = loop_setup(image, loop_n)) < 0)
		return ret;
	return 1;
}
