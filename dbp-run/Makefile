VALAC = valac
VALAPKG = --pkg=gio-2.0 --target-glib 2.32 --pkg glib-2.0 --pkg=gtk+-3.0 --pkg=posix --pkg=dbp
VALAFLAGS = --vapidir=./common -X -I../libdbpbase -X -I../build/include --thread -X -DGETTEXT_PACKAGE=\"dbp-run\" -X -L../build/lib
LIBS = -X -larchive -X -ldbpmgr -X -ldbpbase

BINPATH = ../build/bin

all:
	@echo " [VALA] dbp-run.vala"
	@$(VALAC) -o $(BINPATH)/dbp-run $(VALAPKG) $(VALAFLAGS) dbp-run.vala common/*.vala $(VALASTATICLIBS) $(LIBS)
	@echo " [VALA] dbp-run-path.vala"
	@$(VALAC) -o $(BINPATH)/dbp-run-path $(VALAPKG) $(VALAFLAGS) dbp-run-path.vala common/*.vala $(VALASTATICLIBS) $(LIBS)
	@echo "Done."

clean:
	@echo " Nothing needs to be done to clean dbp-run/"
