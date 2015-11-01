# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(CMDBIN)"
	@$(CC) $(LDPATH) -o $(CMDBIN) $(OBJFILES) $(LDFLAGS) -ldbpbase -ldbpmgr
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbp-cmd/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

