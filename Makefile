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
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/
	@echo " [ CD ] dbp-run-vala/"
	+@make -C dbp-run-vala/
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/
	
	
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
	@echo
	@echo "Source tree cleaned."
	@echo

