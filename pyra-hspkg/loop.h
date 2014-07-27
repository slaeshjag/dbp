#ifndef __LOOP_H__
#define	__LOOP_H__

int loop_mount(const char *image, const char *id, const char *user, const char *src_mount);
int loop_directory_setup(const char *path, int umask);

#endif
