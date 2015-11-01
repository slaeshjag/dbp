#ifndef __LOOP_H__
#define	__LOOP_H__

int loop_desktop_directory(const char *path);
int loop_mount(const char *image, const char *id, const char *user, const char *src_mount, const char *appdata);
int loop_umount(const char *pkg_id, int loop, const char *user, int prev_state);
int loop_directory_setup(const char *path, int umask);

#endif
