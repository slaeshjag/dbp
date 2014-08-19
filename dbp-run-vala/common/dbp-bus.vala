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
