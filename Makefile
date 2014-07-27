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
	@echo " [ CD ] run-dbp/"
	+@make -C run-dbp/
	
	
	@echo "Build complete."
	@echo 

clean:
	@echo " [ RM ] build/"
	+@$(RM) build/
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/ clean
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/ clean
	@echo " [ CD ] run-dbp/"
	+@make -C run-dbp/ clean
	@echo
	@echo "Source tree cleaned."
	@echo

