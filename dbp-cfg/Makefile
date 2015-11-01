# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
LDFLAGS		+=	-ldbpbase
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(CONFIGBIN)"
	@$(CC) -o $(CONFIGBIN) $(OBJFILES) $(LDPATH) $(LDFLAGS)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbp-cfg/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

