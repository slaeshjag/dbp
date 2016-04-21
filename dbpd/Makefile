# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	`pkg-config glib-2.0 --cflags gio-unix-2.0`
LDFLAGS		+=	`pkg-config glib-2.0 --libs gio-unix-2.0` -ldbpbase
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(DAEMONBIN)"
	@$(CC) -o $(DAEMONBIN) $(OBJFILES) $(LDPATH) -ldbpbase $(LDFLAGS)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbpd/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

