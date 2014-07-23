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
	@echo " [ CD ] pyra-hspkg/"
	+@make -C pyra-hspkg/
	@echo " [ CD ] pyra-hspkgd/"
	+@make -C pyra-hspkgd/
	
	
	@echo "Build complete."
	@echo 

clean:
	@echo " [ RM ] build/"
	+@$(RM) build/
	@echo " [ CD ] pyra-hspkg/"
	+@make -C pyra-hspkg/ clean
	@echo " [ CD ] pyra-hspkgd/"
	+@make -C pyra-hspkgd/ clean
	@echo
	@echo "Source tree cleaned."
	@echo

