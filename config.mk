# Project: pyra-hspkg
# Makefile configurations

-include $(TOPDIR)/version.mk

NAME		=	dbp

#Uncomment to build dbp-run without dpkg dependency checking
#BUILD_DPKG	=	NO
DAEMONBIN	=	"$(TOPDIR)/build/bin/$(NAME)d"
LIBBINFILE	=	"lib$(NAME)mgr.so"
LIBBIN		=	"$(TOPDIR)/build/lib/$(LIBBINFILE)"
RUNNERBIN	=	"$(TOPDIR)/build/bin/$(NAME)-run-old"
CONFIGBIN	=	"$(TOPDIR)/build/bin/$(NAME)-cfg"
METABIN		=	"$(TOPDIR)/build/bin/$(NAME)-meta"
CMDBIN		=	"$(TOPDIR)/build/bin/$(NAME)-cmd"
DESKTOPBIN	=	"$(TOPDIR)/build/bin/$(NAME)-desktopd"
THUMBBIN	=	"$(TOPDIR)/build/bin/$(NAME)-thumbnailer"
LIB		=	"$(TOPDIR)/dbp-common/dbp-common.a"
LIBINC		=	"-I$(TOPDIR)/dbp-common" "-I$(TOPDIR)/build/include"

LDPATH		+=	"-L$(TOPDIR)/build/lib"
#DBGFLAGS	=	-O0 -g -D__DEBUG__
DBGFLAGS	=	-O3 -g
CFLAGS		+=	-Wall -Wextra -D_XOPEN_SOURCE=700 -std=c99 $(INCS) $(DBGFLAGS)
LDFLAGS		+=	$(LIB) -larchive -lpthread
RM		=	rm -fR
MKDIR		=	mkdir -p
CP		=	cp
MV		=	mv
PREFIX		?=	$(TOPDIR)/install

ifneq ($(wildcard /etc/debian_version),) 
	#Debian packaging
	ARCH	:=	$(shell dpkg-architecture -qDEB_BUILD_ARCH)
	DEB	:=	$(NAME)-$(VERSION)$(ARCH)
	PACKAGE	:=	$(DEB).deb
#	SECTION	:=	system
	DEPS	=	libc6, libarchive13
endif

ifeq ($(STRIP_INSTALL), true)
	STRIP_FLAG = -s
endif
