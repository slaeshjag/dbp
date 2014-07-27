# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ AR ] $(LIB)"
	@$(AR) -cr $(LIB) $(OBJFILES)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbp-common/$<"
	@$(CC) $(CFLAGS) -c -o $@ $<
	

