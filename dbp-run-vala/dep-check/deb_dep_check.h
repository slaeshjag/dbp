#ifndef __DEB_DEP_CHECK_H__
#define	__DEB_DEP_CHECK_H__

void deb_do_init();
void deb_do_free();
int deb_check_package(const char *pkgname, const char *arch_given);


#endif
