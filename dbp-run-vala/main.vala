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

class ExecLine {
	private string _str;
	private string[] _exec;
	
	public string str {
		get {return _str;}
	}
	public string[] exec {
		get {return _exec;}
	}
	
	private enum ParseState {
		NORMAL,
		QUOTE,
		BACKSLASH,
		PERCENT
	}
	
	public ExecLine(string cmd_line) {
		//Gee.ArrayList<string> arguments = new Gee.ArrayList<string>();
		string[] arguments = {};
		StringBuilder arg = new StringBuilder();
		unichar c;
		ParseState state = ParseState.NORMAL;
		int i;
		
		_str = cmd_line;
		
		for(i = 0; cmd_line.get_next_char (ref i, out c);) {
			switch(state) {
				case ParseState.NORMAL:
				case ParseState.QUOTE:
					switch(c) {
						case '"':
							state = (state == ParseState.QUOTE) ? ParseState.NORMAL : ParseState.QUOTE;
							break;
						
						case '\\':
							state = ParseState.BACKSLASH;
							break;
						
						case '%':
							state = ParseState.PERCENT;
							break;
						
						case ' ':
							if(state == ParseState.QUOTE) {
								arg.append_unichar(c);
							} else {
								if(arg.str.length >0) {
									arguments += arg.str;
								}
								arg = new StringBuilder();
							}
							break;
						
						default:
							arg.append_unichar(c);
							break;
					}
					break;
					
				case ParseState.BACKSLASH:
					arg.append_unichar(c);
					state = ParseState.NORMAL;
					break;
					
				case ParseState.PERCENT:
					switch(c) {
						case '%':
							arg.append_unichar(c);
							break;
					}
					state = ParseState.NORMAL;
					break;
			}
		}
		
		if(arg.str.length > 0) {
			arguments += arg.str;
		}
		
		_exec = arguments;
	}
	
	public void run(bool in_background) {
		if(in_background)
			Process.spawn_async(null, _exec, null, SpawnFlags.SEARCH_PATH | SpawnFlags.CHILD_INHERITS_STDIN, null, null);
		else
			Process.spawn_sync(null, _exec, null, SpawnFlags.SEARCH_PATH | SpawnFlags.CHILD_INHERITS_STDIN, null, null, null);
	}
}

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
		string actual_path;
		string[] argv;
		DBP.Meta.Package meta;
		ExecLine exec;
		
		pkg_id = bus.register_path(path, "", out error_code);
		stdout.printf("rpr %s %s\n", error_code, pkg_id);
		if(pkg_id == "!") {
			stderr.printf("Failed to register path %s\n", path);
			return;
		}
		
		actual_path = bus.path_from_id(pkg_id, "");
		DBP.Meta.package_open(actual_path, out meta);
		exec = new ExecLine(meta.desktop_file.lookup("Exec", "", "Desktop Entry"));
		exec.run(false);
	}
}
int main(string[] args) {
	DBP.Config.init();
	
	bus = null;
	stdout.printf("%s %i\n", DBP.DBus.CLIENT_PREFIX, DBP.Error.BAD_FSIMG);
	try {
		bus = Bus.get_proxy_sync(BusType.SYSTEM, DBP.DBus.DAEMON_PREFIX, DBP.DBus.DAEMON_OBJECT);
		
		//stdout.printf("Path to test package: %s\n", bus.path_from_id("testpkg_slaeshjag", ""));
	} catch(IOError e) {
		//stderr.printf ("%s\n", e.message);
	} catch(Error e) {
		stderr.printf ("%s\n", e.message);
	}
	
	DBP.Desktop desk = new DBP.Desktop.from_file("/tmp/arne.desktop");
	stdout.printf("Firefox is a: %s\n", desk.lookup("GenericName", "", "Desktop Entry"));
	var exec = new ExecLine(desk.lookup("Exec", "", "Desktop Entry"));
	stdout.printf("[\n");
	foreach(string s in exec.exec)
		stdout.printf("\t%s\n", s);
	stdout.printf("]\n");
	exec.run(false);
	
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
