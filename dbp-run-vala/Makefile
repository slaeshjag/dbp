VALAC = valac
VALAPKG = --pkg=gio-2.0 --target-glib 2.32 --pkg glib-2.0 --pkg=posix --pkg=dbp
VALAFLAGS = --vapidir=./common -X -I../dbp-common --thread -X -DGETTEXT_PACKAGE=\"dbp-run\"
VALASTATICLIBS = -X ../dbp-common/dbp-common.a
LIBS = -X -larchive
BINPATH = ../build/bin

all:
	xgettext -L C -k_ -d dbp-run -s -o po/dbp-run.pot *.vala common/*.vala
	make -C po/
	
	$(VALAC) -o $(BINPATH)/dbp-run $(VALAPKG) $(VALAFLAGS) dbp-run.vala common/*.vala -X common/*.c $(VALASTATICLIBS) $(LIBS)
	$(VALAC) -o $(BINPATH)/dbp-run-path $(VALAPKG) $(VALAFLAGS) dbp-run-path.vala common/*.vala -X common/*.c $(VALASTATICLIBS) $(LIBS)

clean:
	@echo " Nothing needs to be done to clean dbp-run-vala/"
