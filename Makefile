# Project: pyra-hspkg
MAKEFLAGS	+=	--no-print-directory

TOPDIR		=	$(shell pwd)
export TOPDIR
include config.mk

.PHONY: all install clean

all:
	@./update_version
	@echo " [INIT] build/"
	@$(MKDIR) build/
	@echo " [INIT] build/bin"
	@$(MKDIR) build/bin/
	@echo " [INIT] build/share/locale"
	@$(MKDIR) build/share/
	@$(MKDIR) build/share/locale
	@echo " [INIT] build/lib"
	@$(MKDIR) build/lib
	@echo " [INIT] build/include/dbpmgr"
	@$(MKDIR) build/include
	@$(MKDIR) build/include/dbpmgr
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/
	@echo " [ CD ] libdbpmgr/"
	+@make -C libdbpmgr/
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/
#	@echo " [ CD ] dbp-thumbnailer/"
#	+@make -C dbp-thumbnailer/
	@echo " [ CD ] dbp-run/"
	+@make -C dbp-run/
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/
	@echo " [ CD ] dbp-cmd/"
	+@make -C dbp-cmd/
	@echo " [ CD ] dbp-desktopd/"
	+@make -C dbp-desktopd/
	
	@echo " [POT ] po/dbp-run.pot"
	@xgettext --package-name="DragonBox-Package-system" --package-version="$(VERSION)" -L C -k_ -d dbp-run -s -o po/dbp-run.pot dbp-run/*.vala dbp-run/common/*.vala dbp-cfg/*.c dbp-meta/*.c dbp-cmd/*.c dbp-thumbnailer/*.c
	@echo " [ CD ] po/"
	+@make -C po/
	
	@echo "Build complete."
	@echo 

clean:
	@echo " [ RM ] build/"
	+@$(RM) build/
	@echo " [ CD ] dbp-common/"
	+@make -C dbp-common/ clean
	@echo " [ CD ] libdbpmgr/"
	+@make -C libdbpmgr/ clean
	@echo " [ CD ] dbpd/"
	+@make -C dbpd/ clean
	@echo " [ CD ] dbp-thumbnailer/"
	+@make -C dbp-thumbnailer/ clean
	@echo " [ CD ] dbp-run/"
	+@make -C dbp-run/ clean
	@echo " [ CD ] dbp-cfg/"
	+@make -C dbp-cfg/ clean
	@echo " [ CD ] dbp-meta/"
	+@make -C dbp-meta/ clean
	@echo " [ CD ] dbp-cmd/"
	+@make -C dbp-cmd/ clean
	@echo " [ CD ] dbp-desktopd/"
	+@make -C dbp-desktopd/ clean
	@echo
	@echo "Source tree cleaned."
	@echo

install:
	install -d "$(PREFIX)/etc/dbp"
	install -m 644 -t "$(PREFIX)/etc/dbp" "$(TOPDIR)/conf/dbp_config.ini"
	install -m 644 -t "$(PREFIX)/etc/dbp" "$(TOPDIR)/conf/dbpd-introspection.xml"
	install -m 644 -t "$(PREFIX)/etc/dbp" "$(TOPDIR)/scripts/dbp_exec"
	install -m 755 -t "$(PREFIX)/etc/dbp" "$(TOPDIR)/scripts/run_script"
	install -d "$(PREFIX)/etc/dbus-1/system.d"
	install -m 644 -t "$(PREFIX)/etc/dbus-1/system.d" "$(TOPDIR)/conf/de.dragonbox.PackageDaemon.conf"
	install -d "$(PREFIX)/etc/xdg/autostart"
	install -m 644 -t "$(PREFIX)/etc/xdg/autostart" "$(TOPDIR)/conf/dbp-desktopd-autostart.desktop"
	install -d "$(PREFIX)/usr/share/binfmts/"
	install -m 644 -t "$(PREFIX)/usr/share/binfmts" "$(TOPDIR)/conf/binfmts/dbp.binfmt"
	install -d "$(PREFIX)/usr/share/applications"
	install -m 644 -t "$(PREFIX)/usr/share/applications" "$(TOPDIR)/conf/dbp-run-path.desktop"
	install -d "$(PREFIX)/usr/share/mime"
	install -m 644 -t "$(PREFIX)/usr/share/mime" "$(TOPDIR)/conf/mime/x-dbp.xml"
	install -d "$(PREFIX)/usr/bin"
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-cfg" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-cmd" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbpd" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-desktopd" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-meta" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-run" 
	install -m 755 $(STRIP_FLAG) -t "$(PREFIX)/usr/bin/" "$(TOPDIR)/build/bin/dbp-run-path" 
	install -d "$(PREFIX)/usr/lib"
	install -m 644 $(STRIP_FLAG) -t "$(PREFIX)/usr/lib/" "$(TOPDIR)/build/lib/libdbpmgr.so"
	install -d "$(PREFIX)/usr/include/dbpmgr"
	install -m 644 -t "$(PREFIX)/usr/include/dbpmgr" $(TOPDIR)/build/include/dbpmgr/*.h
