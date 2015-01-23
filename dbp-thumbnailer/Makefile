# Project: pyra-hspkg
include $(TOPDIR)/config.mk

SRCFILES	=	$(wildcard *.c)
OBJFILES	=	$(SRCFILES:.c=.o)
CFLAGS		+=	`pkg-config --cflags glib-2.0`
LDFLAGS		+=	`pkg-config --libs glib-2.0` -lImlib2
.PHONY: all clean

all: $(OBJFILES) $(DEPENDS)
	@echo " [ LD ] $(THUMBBIN)"
	@$(CC) -o $(THUMBBIN) $(OBJFILES) $(LIB) $(LDFLAGS)
	@echo "Done."
	@echo
	
clean:
	@echo
	@echo " [ RM ] $(OBJFILES)"
	@$(RM) $(OBJFILES)
	@echo "Done."
	@echo 

%.o: %.c %.h
	@echo " [ CC ] dbp-thumbnailer/$<"
	@$(CC) $(LIBINC) $(CFLAGS) -c -o $@ $<
	

