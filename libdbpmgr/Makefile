# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	`pkg-config glib-2.0 --cflags gio-unix-2.0` -shared -fPIC -I$(TOPDIR)/dbp-common
LDFLAGS		+=	`pkg-config glib-2.0 --libs gio-unix-2.0` -Wl,-soname,$(LIBBINFILE)
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(DAEMONBIN)"
	@$(CC) $(CFLAGS) -o $(LIBBIN) $(OBJFILES) $(LIB) $(LDFLAGS)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] libdbpmgr/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

