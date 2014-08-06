//valid package names conform to /[-_\.a-zA-Z0-9]+/

struct RegisterPathReturn {
	string error_code;
	string pkg_id;
}

[DBus (name = "de.dragonbox.PackageDaemon")]
interface DBPBus : Object {
	public abstract string mount(string pkg_id, string username) throws IOError;
	public abstract string u_mount(string id, string dontcare) throws IOError;
	public abstract RegisterPathReturn register_path(string path, string dontcare) throws IOError;
	public abstract string unregister_path(string path, string dontcare) throws IOError;
	public abstract string mount_p(string pkg_id, string dontcare) throws IOError;
	public abstract string path_from_id(string pkg_id, string dontcare) throws IOError;
}

int main(string[] args) {
	DBPBus bus = null;
	stdout.printf("%s %i\n", DBP.DBus.CLIENT_PREFIX, DBP.Error.BAD_FSIMG);
	try {
		bus = Bus.get_proxy_sync(BusType.SYSTEM, DBP.DBus.DAEMON_PREFIX, DBP.DBus.DAEMON_OBJECT);
	} catch(IOError e) {
		stderr.printf ("%s\n", e.message);
	}
	
	DBP.Desktop desk = new DBP.Desktop.from_file("/usr/share/applications/firefox.desktop");
	stdout.printf("%s\n", desk.lookup("GenericName", "", "Desktop Entry"));
	
	DBP.Meta.Package mp;
	DBP.Meta.package_open("/tmp/test.dbp", out mp);
	stdout.printf("%s\n", mp.desktop_file.lookup("Id", "", "Package Entry"));
	
	return 0;
}
