VALAC = valac
VALAPKG = --pkg=gio-2.0 --target-glib 2.32 --pkg glib-2.0 --pkg=gtk+-3.0 --pkg=posix --pkg=dbp
VALAFLAGS = --vapidir=./common -X -I../dbp-common --thread -X -DGETTEXT_PACKAGE=\"dbp-run\"
VALASTATICLIBS = -X ../dbp-common/dbp-common.a
LIBS = -X -larchive -X -ldpkg

ifeq ($(BUILD_DPKG), NO)
	DEPCHECKLIBS =
	DEPCHECKSRC = -X -I./dep-check dep-check/*.vala -X dep-check/deb_dep_check.nodpkg.c
else
	DEPCHECKLIBS = -X -ldpkg
	DEPCHECKSRC = -X -I./dep-check dep-check/*.vala -X dep-check/deb_dep_check.c
endif
BINPATH = ../build/bin

all:
	@echo " [VALA] dbp-run.vala"
	@$(VALAC) -o $(BINPATH)/dbp-run $(VALAPKG) $(VALAFLAGS) dbp-run.vala common/*.vala -X common/*.c $(DEPCHECKSRC) $(VALASTATICLIBS) $(LIBS) $(DEPCHECKLIBS)
	@echo " [VALA] dbp-run-path.vala"
	@$(VALAC) -o $(BINPATH)/dbp-run-path $(VALAPKG) $(VALAFLAGS) dbp-run-path.vala common/*.vala -X common/*.c  $(DEPCHECKSRC) $(VALASTATICLIBS) $(LIBS) $(DEPCHECKLIBS)
	@echo " [VALA] dbp-desktopd.vala"
	@$(VALAC) -o $(BINPATH)/dbp-desktopd $(VALAPKG) $(VALAFLAGS) dbp-desktopd.vala common/dbp-bus.vala common/*.c $(VALASTATICLIBS) $(LIBS)
	#@echo " [VALA] dbp-cmd.vala"
	#@$(VALAC) -o $(BINPATH)/dbp-cmd $(VALAPKG) $(VALAFLAGS) $(VALASTATICLIBS) dbp-cmd.vala common/dbp-bus.vala common/logging.c $(LIBS)
	@echo "Done."

clean:
	@echo " Nothing needs to be done to clean dbp-run/"
