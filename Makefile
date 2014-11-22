# Project: pyra-hspkg
MAKEFLAGS	+=	--no-print-directory

TOPDIR		=	$(shell pwd)
export TOPDIR
include config.mk

.PHONY: all install clean

all:
	@echo " [INIT] build/"
	@$(MKDIR) build/
	@echo " [INIT] build/bin"
	@$(MKDIR) build/bin/
	@echo " [INIT] build/share/locale"
	@$(MKDIR) build/share/
	@$(MKDIR) build/share/locale
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/
	@echo " [ CD ] dbp-run-vala/"
	+@make -C dbp-run-vala/
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/
	
	@echo " [POT ] po/dbp-run.pot"
	@xgettext -L C -k_ -d dbp-run -s -o po/dbp-run.pot dbp-run-vala/*.vala dbp-run-vala/common/*.vala dbp-cfg/*.c dbp-meta/*.c
	@echo " [ CD ] po/"
	@make -C po/
	
	@echo "Build complete."
	@echo 

clean:
	@echo " [ RM ] build/"
	+@$(RM) build/
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/ clean
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/ clean
	@echo " [ CD ] dbp-run-vala/"
	+@make -C dbp-run-vala/ clean
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/ clean
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/ clean
	@echo
	@echo "Source tree cleaned."
	@echo

