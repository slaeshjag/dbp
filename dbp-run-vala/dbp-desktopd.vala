[CCode(cname="GETTEXT_PACKAGE")]
extern const string GETTEXT_PACKAGE;

string desktop_directory;

string? get_desktop() {
	string desktop, home;

	desktop = GLib.Environment.get_variable("XDG_DESKTOP_DIR");
	if (desktop != null)
		return desktop;
	
	/* Didn't find it in XDG. Time to guess? */
	stderr.printf(_("Warning: $XDG_DESKTOP_DIR is not set. Guessing $HOME/Desktop..\n"));
	home = GLib.Environment.get_home_dir();
	if (home == null) {
		/* Crap.. */
		stderr.printf(_("Error: Unable to locate your home directory\n"));
		return null;
	}
	
	desktop = GLib.Path.build_filename(home, "Desktop", null);
	if (!GLib.FileUtils.test(desktop, GLib.FileTest.IS_DIR)) {
		/* Well... Shit. */
		stderr.printf(_("Error: Guessed $HOME/Desktop, which doesn't exist. Configure $XDG_DESKTOP_DIR properly, kthx?\n"));
		return null;
	}

	return desktop;
}


void nuke_desktop() {
	string full;

	try {
		var directory = File.new_for_path(desktop_directory);
		var enumerator = directory.enumerate_children(FileAttribute.STANDARD_NAME, 0);
		
		for (FileInfo file_info = enumerator.next_file(); file_info != null; file_info = enumerator.next_file()) {
			full = file_info.get_name();
			if (!full.has_prefix("__dbp__"))
				continue;
			if (!full.has_suffix(".desktop"))
				continue;
			full = GLib.Path.build_filename(desktop_directory, file_info.get_name());
			GLib.FileUtils.unlink(full);
		}
	} catch (Error e) {
		stderr.printf(_("Error: %s\n"), e.message);
		return;
	}
	

	return;
}

string desktop_path(string full_path) {
	string component;
	string desktop_path_s;

	component = GLib.Path.get_basename(full_path);
	desktop_path_s = GLib.Path.build_filename(desktop_directory, component, null);
	return desktop_path_s;
}

void add_meta(string path) {
	var src = File.new_for_path(path);
	var dest = File.new_for_path(desktop_path(path));
	src.copy(dest, FileCopyFlags.NONE);
	return;
}

void remove_meta(string path) {
	GLib.FileUtils.unlink(desktop_path(path));
	return;
}


/* This is a bit of a hack... */
void add_package_meta(string pkgid) {
	string fname, prefix, path, newpath;

	prefix = "__dbp__" + pkgid + "_";

	try {
		var directory = File.new_for_path(DBP.Config.config.desktop_directory);
		var enumerator = directory.enumerate_children(FileAttribute.STANDARD_NAME, 0);

		for (FileInfo file_info = enumerator.next_file(); file_info != null; file_info = enumerator.next_file()) {
			fname = file_info.get_name();
			if (!fname.has_prefix(prefix))
				continue;
			if (!fname.has_suffix(".desktop"))
				continue;
			path = GLib.Path.build_filename(DBP.Config.config.desktop_directory, fname, null);
			newpath = GLib.Path.build_filename(desktop_directory, fname, null);
			var src = File.new_for_path(path);
			var dest = File.new_for_path(newpath);
			src.copy(dest, FileCopyFlags.NONE);
		}
	} catch (Error e) {
		stderr.printf(_("Error: %s\n"), e.message);
		return;
	}

	return;
}


int main(string[] args) {
	string[] package_list;
	
	DBP.Config.init();
	desktop_directory = get_desktop();
	if (desktop_directory == null)
		return 1;
	
	nuke_desktop();
	try {
		bus = Bus.get_proxy_sync(BusType.SYSTEM, DBP.DBus.DAEMON_PREFIX, DBP.DBus.DAEMON_OBJECT);
	} catch(Error e) {
		stderr.printf (_("Error: %s\n"), e.message);
		return 1;
	}

	bus.new_meta.connect(add_meta);
	bus.remove_meta.connect(remove_meta);

	try {
		package_list = bus.package_list();
		for (int i = 0; i < package_list.length/3; i++) {
			if (package_list[i*3 + 2] == "desk") {
				add_package_meta(package_list[i*3 + 1]);
			}
		}
	} catch (Error e) {
		return 1;
	}

	var loop = new MainLoop();
	loop.run();

	return 0;
}
