# Project: pyra-hspkg
# Makefile configurations

NAME		=	pyra-hspkg
VERSION		=	0.1

DAEMONBIN	=	"$(TOPDIR)/build/bin/$(NAME)d"
RUNNERBIN	=	"$(TOPDIR)/build/bin/$(NAME)-run"
LIB		=	"$(TOPDIR)/pyra-hspkg/pyra-hspkg.a"
LIBINC		=	"-I$(TOPDIR)/pyra-hspkg"
PREFIX		+=	/usr/local

DBGFLAGS	=	-O0 -g -D__DEBUG__
#DBGFLAGS	=	-O3 -g
CFLAGS		+=	-Wall $(INCS) $(DBGFLAGS) `pkg-config --cflags dbus-1`
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

