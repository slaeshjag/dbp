# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	`pkg-config glib-2.0 --cflags gio-unix-2.0` -shared -fPIC -I$(TOPDIR)/libdbpbase
LDFLAGS		+=	`pkg-config glib-2.0 --libs gio-unix-2.0` -Wl,-soname,$(LIBBINFILE) -ldbpbase
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(LIBBIN)"
	@$(CC) $(CFLAGS) -o $(LIBBIN) $(LDPATH) $(OBJFILES) $(LDFLAGS)
	@echo " [COPY] Headers into build directory"
	@$(CP) *.h $(TOPDIR)/build/include/dbpmgr/
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
	

