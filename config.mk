# Project: pyra-hspkg
# Makefile configurations

-include $(TOPDIR)/version.mk

NAME		=	dbp

DAEMONBIN	=	"$(TOPDIR)/build/bin/$(NAME)d"
LIBBINFILE	=	"lib$(NAME)mgr.so"
LIBBIN		=	"$(TOPDIR)/build/lib/$(LIBBINFILE)"
LIBBASEFILE	=	"lib$(NAME)base.so"
LIBBASE		=	"$(TOPDIR)/build/lib/$(LIBBASEFILE)"
RUNNERBIN	=	"$(TOPDIR)/build/bin/$(NAME)-run-old"
CONFIGBIN	=	"$(TOPDIR)/build/bin/$(NAME)-cfg"
METABIN		=	"$(TOPDIR)/build/bin/$(NAME)-meta"
CMDBIN		=	"$(TOPDIR)/build/bin/$(NAME)-cmd"
DESKTOPBIN	=	"$(TOPDIR)/build/bin/$(NAME)-desktopd"
THUMBBIN	=	"$(TOPDIR)/build/bin/$(NAME)-thumbnailer"
VALIDBIN	=	"$(TOPDIR)/build/bin/$(NAME)-validate-extracted"
LIB		=	"$(TOPDIR)/dbp-common/dbp-common.a"
LIBINC		=	"-I$(TOPDIR)/libdbpbase" "-I$(TOPDIR)/build/include"

LDPATH		+=	"-L$(TOPDIR)/build/lib"
DBGFLAGS	=	-O0 -g -D__DEBUG__
#DBGFLAGS	=	-O3 -g
CFLAGS		+=	-Wall -Wextra -D_XOPEN_SOURCE=700 -std=c99 $(INCS) $(DBGFLAGS)
LDFLAGS		+=	-larchive -lpthread
RM		=	rm -fR
MKDIR		=	mkdir -p
CP		=	cp
MV		=	mv
DESTDIR		?=	/
PREFIX		?=	$(DESTDIR)

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
