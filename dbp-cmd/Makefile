# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(METABIN)"
	@$(CC) $(LDPATH) -o $(CMDBIN) $(OBJFILES) $(LIB) $(LDFLAGS) -ldbpmgr
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
	

