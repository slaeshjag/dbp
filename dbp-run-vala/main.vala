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

int main(string[] args) {
	DBP.Config.init();
	
	bus = null;
	stdout.printf("%s %i\n", DBP.DBus.CLIENT_PREFIX, DBP.Error.BAD_FSIMG);
	try {
		bus = Bus.get_proxy_sync(BusType.SYSTEM, DBP.DBus.DAEMON_PREFIX, DBP.DBus.DAEMON_OBJECT);
		
		stdout.printf("Path to test package: %s\n", bus.path_from_id("testpkg_slaeshjag", ""));
	} catch(IOError e) {
		stderr.printf ("%s\n", e.message);
		return 1;
	} catch(Error e) {
		stderr.printf ("%s\n", e.message);
		return 1;
	}
	
	/*DBP.Desktop desk = new DBP.Desktop.from_file("/tmp/arne.desktop");
	stdout.printf("Firefox is a: %s\n", desk.lookup("GenericName", "", "Desktop Entry"));
	var exec = new ExecLine(desk.lookup("Exec", "", "Desktop Entry"));
	exec.run(false);
	
	DBP.Meta.Package mp;
	DBP.Meta.package_open("/tmp/test.dbp", out mp);
	stdout.printf("%s\n", mp.desktop_file.lookup("Id", "", "Package Entry"));
	
	stdout.printf("Search dirs:\n");
	foreach(string dir in DBP.Config.config.search_dir)
		stdout.printf("\t%s\n", dir);
	
	Run.run_path("/tmp/test.dbp");
	//Run.appdata_create("testpkg_slaeshjag");
	*/
	
	Run.run("testpkg_slaeshjag", "meta/test", {}, true, false);
	
	
	return 0;
}
