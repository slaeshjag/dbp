namespace Run {
	const int appdata_mode = 0755;
	
	class OutputLogger {
		int fd;
		string filename;
		unowned FileStream stream;
		Thread<int> thread;
		
		public OutputLogger(int fd, string filename, FileStream stream) {
			this.filename = filename;
			this.fd = fd;
			this.stream = stream;
			thread = new Thread<int>("output-logger", run);
		}
		
		private int run() {
			FileStream f = FileStream.open(filename, "wb");
			ssize_t bytes;
			uint8 buf[256];
			
			while(true) {
				bytes = Posix.read(fd, buf, buf.length);
				if(bytes <= 0)
					break;
				f.write(buf[0:bytes], 1);
				stream.write(buf[0:bytes], 1);
			}
			Posix.close(fd);
			return 0;
		}
	}
	
	void appdata_create(string pkg_id) throws IOError {
		string mountpoint;
		string appdata, roappdata;
		
		mountpoint = bus.mount_point_get(pkg_id);
		
		if(mountpoint == null || mountpoint == "" || mountpoint == "!")
			throw new IOError.FAILED(_("Failed to find mountpoint"));
		
		if(DBP.Config.config.per_user_appdata) {
			appdata = DBP.Config.config.data_directory + "_" + Environment.get_user_name();
			roappdata = DBP.Config.config.rodata_directory + "_" + Environment.get_user_name();
		} else {
			appdata = DBP.Config.config.data_directory;
			roappdata = DBP.Config.config.rodata_directory;
		}
	
		if (DBP.Config.config.per_package_appdata) {
			DirUtils.create_with_parents(Path.build_filename(mountpoint, appdata, pkg_id), appdata_mode);
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata, pkg_id), appdata_mode);
		} else {
			DirUtils.create_with_parents(Path.build_filename(mountpoint, appdata), appdata_mode);
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata), appdata_mode);
		}
	}
	
	public void run(string pkg_id, string exec, string[] args, bool log, bool chdir) throws IOError, SpawnError {
		string mount_id;
		string binary_path;
		string ?cwd;
		string[] argv = {};
		int outpipe, errpipe;
		string outlogfile, errlogfile;
		int pid;
		int info;
		OutputLogger stdoutlogger, stderrlogger;
		
		appdata_create(pkg_id);
		
		//TODO: validate pkg_id
		
		mount_id = bus.mount(pkg_id, "");
		if(int.parse(mount_id) < 0)
			throw new IOError.FAILED(mount_id);
		
		binary_path = Path.build_filename(DBP.Config.config.union_mount, pkg_id, exec);
		cwd = chdir ? Path.build_filename(DBP.Config.config.union_mount, pkg_id) : null;
		
		argv += binary_path;
		foreach(string s in args)
			argv += s;
		
		if(log) {
			Process.spawn_async_with_pipes(cwd, argv, null, SpawnFlags.CHILD_INHERITS_STDIN | SpawnFlags.DO_NOT_REAP_CHILD, null, out pid, null, out outpipe, out errpipe);
			
			outlogfile = "%s-%s-stdout%s".printf(Path.build_filename(DBP.Config.config.dbpout_directory, DBP.Config.config.dbpout_prefix), pkg_id, DBP.Config.config.dbpout_suffix);
			errlogfile = "%s-%s-stderr%s".printf(Path.build_filename(DBP.Config.config.dbpout_directory, DBP.Config.config.dbpout_prefix), pkg_id, DBP.Config.config.dbpout_suffix);
			stdoutlogger = new OutputLogger(outpipe, outlogfile, stdout);
			stderrlogger = new OutputLogger(errpipe, errlogfile, stderr);
			
			Posix.waitpid(pid, out info, 0);
			Process.close_pid(pid);
		} else {
			Process.spawn_sync(cwd, argv, null, SpawnFlags.CHILD_INHERITS_STDIN, null, null, null);
		}
		
		bus.u_mount(mount_id);
	}
	
	public void run_path(string path, string[] args) throws IOError, SpawnError {
		string pkg_id;
		string error_code;
		string actual_path;
		DBP.Meta.Package meta;
		ExecLine exec;
		
		pkg_id = bus.register_path(path, out error_code);
		if(pkg_id == "!")
			throw new IOError.FAILED(error_code);
	
		actual_path = bus.path_from_id(pkg_id);
		DBP.Meta.package_open(actual_path, out meta);
		exec = new ExecLine(meta.desktop_file.lookup("Exec", "", "Desktop Entry"));
		exec.append(args);
		exec.run(false);
		
		if(int.parse(error_code) != DBP.Error.PKG_REG)
			bus.unregister_path(path);
	}
}
