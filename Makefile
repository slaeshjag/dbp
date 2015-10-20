# Project: pyra-hspkg
MAKEFLAGS	+=	--no-print-directory

TOPDIR		=	$(shell pwd)
export TOPDIR
include config.mk

.PHONY: all install clean

all:
	@./update_version
	@echo " [INIT] build/"
	@$(MKDIR) build/
	@echo " [INIT] build/bin"
	@$(MKDIR) build/bin/
	@echo " [INIT] build/share/locale"
	@$(MKDIR) build/share/
	@$(MKDIR) build/share/locale
	@echo " [INIT] build/lib"
	@$(MKDIR) build/lib
	@echo " [INIT] build/include/dbpmgr"
	@$(MKDIR) build/include
	@$(MKDIR) build/include/dbpmgr
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/
	@echo " [ CD ] libdbpmgr/"
	+@make -C libdbpmgr/
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/
#	@echo " [ CD ] dbp-thumbnailer/"
#	+@make -C dbp-thumbnailer/
	@echo " [ CD ] dbp-run/"
	+@make -C dbp-run/
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/
	@echo " [ CD ] dbp-cmd/"
	+@make -C dbp-cmd/
	@echo " [ CD ] dbp-desktopd/"
	+@make -C dbp-desktopd/
	
	@echo " [POT ] po/dbp-run.pot"
	@xgettext --package-name="DragonBox-Package-system" --package-version="$(VERSION)" -L C -k_ -d dbp-run -s -o po/dbp-run.pot dbp-run/*.vala dbp-run/common/*.vala dbp-cfg/*.c dbp-meta/*.c
	@echo " [ CD ] po/"
	+@make -C po/
	
	@echo "Build complete."
	@echo 

clean:
	@echo " [ RM ] build/"
	+@$(RM) build/
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/ clean
	@echo " [ CD ] libdbpmgr/"
	+@make -C libdbpmgr/ clean
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/ clean
	@echo " [ CD ] dbp-thumbnailer/"
	+@make -C dbp-thumbnailer/ clean
	@echo " [ CD ] dbp-run/"
	+@make -C dbp-run/ clean
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/ clean
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/ clean
	@echo " [ CD ] dbp-cmd/"
	+@make -C dbp-cmd/ clean
	@echo " [ CD ] dbp-desktopd/"
	+@make -C dbp-desktopd/ clean
	@echo
	@echo "Source tree cleaned."
	@echo

