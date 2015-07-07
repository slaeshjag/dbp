namespace Run {
	const int appdata_mode = 0755;

	string? package_path = null;
	string? package_id = null;
	
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
				stream.flush();
			}
			Posix.close(fd);
			return 0;
		}
	}

	bool dependency_ok(string pkg_id, DBP.Meta.Package pkg) throws IOError {
		string[] sysdeps, dbpdeps, sysmissing, dbpmissing;
		string sysdep, dbpdep, pkgarch;
		
		sysdep = pkg.desktop_file.lookup("SysDependency", "", "Package Entry");
		dbpdep = pkg.desktop_file.lookup("PkgDependency", "", "Package Entry");
		pkgarch = pkg.desktop_file.lookup("Arch", "", "Package Entry");
		if (pkgarch == null || pkgarch == "")
			pkgarch = "any";

		if (sysdep != null) {
			sysdeps = sysdep.split(";");
			sysmissing = DepCheck.check_sys_dep(sysdeps, pkgarch);
		} else
			sysmissing = {};
		
		if (dbpdep != null) {
			dbpdeps = dbpdep.split(";");
			dbpmissing = DepCheck.check_dbp_dep(dbpdeps);
		} else
			dbpmissing = {};
		if (dbpmissing.length > 0 || sysmissing.length > 0) {
			string err;

			err = _("Package '%s' is missing some of its dependencies\n\n").printf(pkg_id);
			if (dbpmissing.length > 0) {
				err = err + _("Removable packages: ");
				err = err + string.joinv(", ", dbpmissing) + "\n\n";
			}

			if (sysmissing.length > 0) {
				err = err + _("System packages: ");
				err = err + string.joinv(", ", sysmissing) + "\n\n";
			}

			err = err + _("The application may or may not work at all without these. It is recommended that you use a package manager to install the missing dependencies.");

			stdout.printf("%s\n", err);
			/* TODO: Ask the user what to do */
		}
			
		return true;	
	}

	string resolve_appdata(string pkg_id, DBP.Meta.Package pkg) {
		string appdata_name; 
		
		appdata_name = pkg.desktop_file.lookup("Appdata", "", "Package Entry");
		
		if (appdata_name == null)
			appdata_name = pkg_id;

		return appdata_name;
	}

	void appdata_create(string pkg_id, string appdata_name) throws IOError {
		string mountpoint;
		string appdata, roappdata;
		
		
		mountpoint = bus.mount_point_get(pkg_id);
		if(mountpoint == null || mountpoint == "" || mountpoint == "!")
			throw new IOError.FAILED(_("Failed to find the mountpoint that this package resides on"));
		
		if(DBP.Config.config.per_user_appdata) {
			appdata = DBP.Config.config.data_directory + "_" + Environment.get_user_name();
			roappdata = DBP.Config.config.rodata_directory + "_" + Environment.get_user_name();
		} else {
			appdata = DBP.Config.config.data_directory;
			roappdata = DBP.Config.config.rodata_directory;
		}
	
		if (DBP.Config.config.per_package_appdata) {
			var dir = Path.build_filename(mountpoint, appdata, appdata_name);
			DirUtils.create_with_parents(dir, appdata_mode);
			if (!FileUtils.test(dir, FileTest.IS_DIR))
				throw new IOError.FAILED(_("Unable to create appdata directory") + " " + dir + "\n" + _("Make sure you have the file system permissions to create this directory"));
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata, appdata_name), appdata_mode);
		} else {
			var dir = Path.build_filename(mountpoint, appdata);
			DirUtils.create_with_parents(dir, appdata_mode);
			if (!FileUtils.test(dir, FileTest.IS_DIR))
				throw new IOError.FAILED(_("Unable to create appdata directory") + " " + dir + "\n" + _("Make sure you have the file system permissions to create this directory"));
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata), appdata_mode);
		}
	}
	
	public void run(string pkg_id, string exec, string[] args, bool log, bool chdir) throws IOError, SpawnError, Error {
		string mount_id;
		string binary_path;
		string ?cwd;
		string[] argv = {};
		string appdata_name, pkgpath, run_script, pkg_arch;
		int outpipe, errpipe;
		string outlogfile, errlogfile;
		int pid;
		int info;
		DBP.Meta.Package pkg;
		OutputLogger stdoutlogger, stderrlogger;
		
		if (pkg_id == null)
			return;
	
		try {
			pkgpath = bus.path_from_id(pkg_id);
		} catch (Error e) {
			throw new IOError.FAILED(_("Unable to connect to connect to dbpd, make sure dbpd is running\n\n" + e.message));
		}
	
		if (pkgpath == null || pkgpath == "" || pkgpath == "!")
			throw new IOError.FAILED(_("pkgid is not in the database"));
	
		Run.package_path = pkgpath;
		DBP.Meta.package_open(pkgpath, out pkg);
		
		if (!dependency_ok(pkg_id, pkg))
			return;

		appdata_name = resolve_appdata(pkg_id, pkg);
		appdata_create(pkg_id, appdata_name);
		
		//TODO: validate pkg_id
		pkg_arch = pkg.desktop_file.lookup("Arch", "", "Package Entry");
		if (pkg_arch == "any" || pkg_arch == null)
			pkg_arch = "";
		run_script = DBP.Config.config.df.lookup("run_script", pkg_arch, "Package Daemon Config");
		if (run_script == "")
			run_script = null;

		mount_id = bus.mount(pkg_id, "");
		if(int.parse(mount_id) < 0)
			throw new IOError.FAILED(mount_id);
	
		binary_path = Path.build_filename(DBP.Config.config.union_mount, pkg_id, exec);
		cwd = chdir ? Path.build_filename(DBP.Config.config.union_mount, pkg_id) : null;
		
		if (run_script != null)
			argv += DBP.Config.config.run_script;

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
	
	public void run_path(string path_in, string[] args) throws IOError, SpawnError, Error {
		string pkg_id;
		string error_code;
		string actual_path;
		string exec_name;
		string path;
		DBP.Meta.Package meta;
		ExecLine exec;
	
		if (path_in[0] != '/')
			path = Path.build_filename(Environment.get_current_dir(), path_in, null);
		else
			path = path_in;

		try {
			pkg_id = bus.register_path(path, out error_code);
		} catch (Error e) {
			throw new IOError.FAILED(_("Unable to connect to connect to dbpd, make sure dbpd is running\n\n" + e.message));
		}
		if(pkg_id == "!")
			throw new IOError.FAILED(error_code);
		
		Run.package_id = pkg_id;
		actual_path = bus.path_from_id(pkg_id);
		Run.package_path = actual_path;
		DBP.Meta.package_open(actual_path, out meta);
		exec_name = meta.desktop_file.lookup("Exec", "", "Desktop Entry");
		if (exec_name == null)
			throw new IOError.FAILED(_("Unable to extract exec from package meta data"));
		exec = new ExecLine(exec_name);
		exec.append(args);
		exec.run(false);
		
		if(int.parse(error_code) != DBP.Error.PKG_REG)
			bus.unregister_path(path);
	}
}
