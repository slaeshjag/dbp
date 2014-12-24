# Project: pyra-hspkg
# Makefile configurations

NAME		=	dbp
VERSION		=	0.5

#Uncomment to build dbp-run without dpkg dependency checking
#BUILD_DPKG	=	NO
DAEMONBIN	=	"$(TOPDIR)/build/bin/$(NAME)d"
RUNNERBIN	=	"$(TOPDIR)/build/bin/$(NAME)-run-old"
CONFIGBIN	=	"$(TOPDIR)/build/bin/$(NAME)-cfg"
METABIN		=	"$(TOPDIR)/build/bin/$(NAME)-meta"
LIB		=	"$(TOPDIR)/dbp-common/dbp-common.a"
LIBINC		=	"-I$(TOPDIR)/dbp-common"
PREFIX		+=	/usr/local

DBGFLAGS	=	-O0 -g -D__DEBUG__
#DBGFLAGS	=	-O3 -g
CFLAGS		+=	-Wall -Wextra -D_XOPEN_SOURCE=700 -std=c99 $(INCS) $(DBGFLAGS) `pkg-config --cflags dbus-1`
LDFLAGS		+=	$(LIB) -larchive -lpthread -ldbus-1
RM		=	rm -fR
MKDIR		=	mkdir -p
CP		=	cp
MV		=	mv

ifneq ($(wildcard /etc/debian_version),) 
	#Debian packaging
	ARCH	:=	$(shell dpkg-architecture -qDEB_BUILD_ARCH)
	DEB	:=	$(NAME)-$(VERSION)$(ARCH)
	PACKAGE	:=	$(DEB).deb
#	SECTION	:=	system
	DEPS	=	libc6, libarchive13
endif

