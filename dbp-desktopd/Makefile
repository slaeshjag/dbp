# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	-D_GNU_SOURCE
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(DESKTOPBIN)"
	@$(CC) $(LDPATH) -o $(DESKTOPBIN) $(OBJFILES) $(LDFLAGS) -ldbpbase -ldbpmgr
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbp-desktopd/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

