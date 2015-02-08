#ifndef __DEB_DEP_CHECK_H__
#define	__DEB_DEP_CHECK_H__

void deb_do_init();
void deb_do_free();
int dbp_deb_dep_check_check_package(const char *pkgname, const char *wanted_arch);


#endif
