VALAC = valac
VALAPKG = --pkg=gio-2.0 --pkg=dbp
VALAFLAGS = --vapidir=. -X -I../dbp-common
VALASTATICLIBS = ../dbp-common/dbp-common.a
LIBS = -larchive
BIN = ../build/bin/dbp-run-vala

all:
	$(VALAC) -o $(BIN) $(VALAPKG) $(VALAFLAGS) *.vala -X *.c -X $(VALASTATICLIBS) -X $(LIBS)
