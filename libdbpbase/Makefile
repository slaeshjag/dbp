# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	-shared -fPIC
LDFLAGS		+=	-Wl,-soname,$(LIBBASEFILE)
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(LIBBASE)"
	@$(CC) $(CFLAGS) -o $(LIBBASE) $(OBJFILES) $(LDFLAGS)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo " [ RM ] $(LIB)"
	@$(RM) $(LIB)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] libdbpbase/$<"
	@$(CC) $(CFLAGS) -c -o $@ $<
	

