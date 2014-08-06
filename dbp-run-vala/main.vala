//valid package names conform to /[-_\.a-zA-Z0-9]+/

[DBus (name = "de.dragonbox.PackageDaemon")]
interface DBPBus : Object {
	public abstract string mount(string pkg_id, string dontcare) throws IOError;
	public abstract string u_mount(string id, string dontcare) throws IOError;
	public abstract string register_path(string path, string dontcare, out string error_code) throws IOError;
	public abstract string unregister_path(string path, string dontcare) throws IOError;
	public abstract string mount_p(string pkg_id, string dontcare) throws IOError;
	public abstract string path_from_id(string pkg_id, string dontcare) throws IOError;
}

DBPBus bus;

namespace Run {
	const int appdata_mode = 0755;
	
	void appdata_create(string pkg_id) {
		string mountpoint;
		string appdata;
		
		try {
			mountpoint = bus.mount_p(pkg_id, "");
		} catch(IOError e) {
			stderr.printf ("%s\n", e.message);
			return;
		}
		
		if(mountpoint == null || mountpoint == "")
			return;
		
		if(DBP.Config.config.per_user_appdata)
			appdata = DBP.Config.config.data_directory + "_" + Environment.get_user_name();
		else
			appdata = DBP.Config.config.data_directory;
		
		stdout.printf("creating appdata: %s\n", Path.build_filename(mountpoint, appdata));
		DirUtils.create_with_parents(Path.build_filename(mountpoint, appdata), appdata_mode);
	}
	
	void run_path(string path, string[] ?arguments) {
		string pkg_id;
		string error_code;
		string exe;
		string actual_path;
		string[] argv;
		DBP.Meta.Package meta;
		
		pkg_id = bus.register_path(path, "", out error_code);
		stdout.printf("rpr %s %s\n", error_code, pkg_id);
		if(pkg_id == "!") {
			stderr.printf("Failed to register path %s\n", path);
			return;
		}
		
		actual_path = bus.path_from_id(pkg_id, "");
		DBP.Meta.package_open(actual_path, out meta);
		exe = meta.desktop_file.lookup("Exec", "", "Desktop Entry");
		stdout.printf("Preparing to run \"%s\"\n", exe);
		
		argv = {Path.get_basename(exe)};
		if(arguments != null)
			foreach(string arg in arguments)
				argv += arg;
		
		//do this shit: http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html
		Process.spawn_sync(null, argv, null, SpawnFlags.SEARCH_PATH | SpawnFlags.CHILD_INHERITS_STDIN, null, null, null);
	}
}
int main(string[] args) {
	DBP.Config.init();
	
	bus = null;
	stdout.printf("%s %i\n", DBP.DBus.CLIENT_PREFIX, DBP.Error.BAD_FSIMG);
	try {
		bus = Bus.get_proxy_sync(BusType.SYSTEM, DBP.DBus.DAEMON_PREFIX, DBP.DBus.DAEMON_OBJECT);
		
		stdout.printf("Path to test package: %s\n", bus.path_from_id("testpkg_slaeshjag", ""));
	} catch(IOError e) {
		stderr.printf ("%s\n", e.message);
	}
	
	DBP.Desktop desk = new DBP.Desktop.from_file("/usr/share/applications/firefox.desktop");
	stdout.printf("Firefox is a: %s\n", desk.lookup("GenericName", "", "Desktop Entry"));
	
	DBP.Meta.Package mp;
	DBP.Meta.package_open("/tmp/test.dbp", out mp);
	stdout.printf("%s\n", mp.desktop_file.lookup("Id", "", "Package Entry"));
	
	stdout.printf("Search dirs:\n");
	foreach(string dir in DBP.Config.config.search_dir)
		stdout.printf("\t%s\n", dir);
	
	Run.run_path("/tmp/test.dbp", null);
	//Run.appdata_create("testpkg_slaeshjag");
	
	return 0;
}
