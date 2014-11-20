VALAC = valac
VALAPKG = --pkg=gio-2.0 --target-glib 2.32 --pkg glib-2.0 --pkg=posix --pkg=dbp
VALAFLAGS = --vapidir=./common -X -I../dbp-common -X -I./common --thread -X -DGETTEXT_PACKAGE=\"dbp-run\"
VALASTATICLIBS = -X ../dbp-common/dbp-common.a
LIBS = -X -larchive -X -ldpkg
DEPCHECKLIBS = -X -ldpkg
DEPCHECKSRC = -X -I./dep-check dep-check/*.vala -X dep-check/*.c
BINPATH = ../build/bin

all:
	xgettext -L C -k_ -d dbp-run -s -o po/dbp-run.pot *.vala common/*.vala
	make -C po/
	
	$(VALAC) -o $(BINPATH)/dbp-run $(VALAPKG) $(VALAFLAGS) dbp-run.vala common/*.vala -X common/*.c $(DEPCHECKSRC) $(VALASTATICLIBS) $(LIBS) $(DEPCHECKLIBS)
	$(VALAC) -o $(BINPATH)/dbp-run-path $(VALAPKG) $(VALAFLAGS) dbp-run-path.vala common/*.vala -X common/*.c  $(DEPCHECKSRC) $(VALASTATICLIBS) $(LIBS) $(DEPCHECKLIBS)
	$(VALAC) -o $(BINPATH)/dbp-desktopd $(VALAPKG) $(VALAFLAGS) dbp-desktopd.vala common/dbp-bus.vala common/*.c $(VALASTATICLIBS) $(LIBS)
	$(VALAC) -o $(BINPATH)/dbp-cmd $(VALAPKG) $(VALAFLAGS) dbp-cmd.vala common/dbp-bus.vala $(LIBS)

clean:
	@echo " Nothing needs to be done to clean dbp-run-vala/"
