#ifndef __DBP_LOOP_H__
#define	__DBP_LOOP_H__

int dbp_loop_desktop_directory(const char *path);
int dbp_loop_mount(const char *image, const char *id, const char *user, const char *src_mount, const char *appdata);
int dbp_loop_umount(const char *pkg_id, int loop, const char *user, int prev_state);
int dbp_loop_directory_setup(const char *path, int umask);

#endif
